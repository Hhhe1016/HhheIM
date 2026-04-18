import socket
import struct
import json
import threading
import time
from enum import IntEnum

# ⭐ 新增：定义与 C++ 完全一致的强类型枚举
class MsgType(IntEnum):
    Login = 1
    Chat = 2
    Heartbeat = 3
    Ack = 4
    SendFailed = 5

# --- 全局状态记录 ---
connected_clients = {}
clients_lock = threading.Lock()

# --- QoS (服务质量保障) 核心组件 ---
offline_messages = {}  # 离线消息队列: {"UserB": [msg_dict1, msg_dict2]}
unacked_messages = {}  # 未确认消息池: {"msgId": {"msg": dict, "timestamp": float, "retries": int}}
qos_lock = threading.Lock() # QoS 专属线程锁

# 辅助函数：统一处理封包和发送逻辑
def send_msg_to_client(conn, msg_dict):
    try:
        body = json.dumps(msg_dict).encode('utf-8')
        header = struct.pack('>I', len(body))
        conn.sendall(header + body)
        return True
    except Exception as e:
        print(f"[-] 发送数据失败: {e}")
        return False

# 后台超时重传引擎 (独立线程运行)
def qos_worker():
    while True:
        time.sleep(2) # 每 2 秒巡视一次
        current_time = time.time()
        
        with qos_lock:
            # 遍历所有未收到 ACK 的消息
            for msg_id, info in list(unacked_messages.items()):
                # 如果发出去超过 5 秒还没收到 ACK
                if current_time - info["timestamp"] > 5.0:
                    if info["retries"] < 3: # 最大重传 3 次
                        info["retries"] += 1
                        info["timestamp"] = current_time # 重置倒计时
                        target = info["msg"].get("to")
                        print(f"[QoS重传] 消息 [{msg_id}] 疑似丢失，正在进行第 {info['retries']} 次重发给 {target}")
                        
                        # 尝试重发
                        with clients_lock:
                            target_conn = connected_clients.get(target)
                        if target_conn:
                            send_msg_to_client(target_conn, info["msg"])
                    else:
                        # 超过 3 次都没回音，认定对方彻底掉线
                        target = info["msg"].get("to")
                        print(f"[QoS放弃] 消息 [{msg_id}] 连续 3 次重发失败，转入离线队列等待 {target} 上线。")
                        if target not in offline_messages:
                            offline_messages[target] = []
                        offline_messages[target].append(info["msg"])
                        # 从待确认池中移除
                        del unacked_messages[msg_id]

def handle_client(conn, addr):
    print(f"[+] 接受来自 {addr} 的新连接")
    current_user = None
    
    try:
        while True:
            header = conn.recv(4)
            if not header: break
            msg_length = struct.unpack('>I', header)[0]
            body = conn.recv(msg_length)
            data = json.loads(body.decode('utf-8'))
            
            # ⭐ 安全提取类型，并转为枚举以防未知数字
            raw_type = data.get("type")
            try:
                msg_type = MsgType(raw_type)
            except ValueError:
                print(f"[-] 收到未知或非法的消息类型: {raw_type}")
                continue

            sender = data.get("from")

            # ⭐ 彻底消灭魔法数字，使用极其清晰的枚举判断
            # --- 类型 1：登录请求 ---
            if msg_type == MsgType.Login:
                with clients_lock:
                    connected_clients[sender] = conn
                current_user = sender
                print(f"[*] 用户上线: {sender}. 当前在线人数: {len(connected_clients)}")
                
                # 登录成功后，检查有没有他的离线消息
                with qos_lock:
                    if sender in offline_messages and offline_messages[sender]:
                        print(f"[*] 发现 {sender} 的离线消息，正在疯狂推送...")
                        for offline_msg in offline_messages[sender]:
                            # 推送离线消息
                            send_msg_to_client(conn, offline_msg)
                            # 推送完不能不管，也要扔进未确认池子里监控起来！
                            unacked_messages[offline_msg['msgId']] = {
                                "msg": offline_msg,
                                "timestamp": time.time(),
                                "retries": 0
                            }
                        # 推送完清空该用户的离线队列
                        offline_messages[sender] = []

            # --- 类型 2：聊天消息 ---
            elif msg_type == MsgType.Chat:
                target = data.get("to")
                msg_id = data.get("msgId")
                print(f"[-] 收到转发请求: {sender} -> {target}, 内容: {data.get('msg')}")
                
                with clients_lock:
                    target_conn = connected_clients.get(target)
                
                with qos_lock:
                    if target_conn:
                        # 1. 目标在线：直接发送
                        send_msg_to_client(target_conn, data)
                        # 2. 扔进未确认池子，开始计时
                        unacked_messages[msg_id] = {
                            "msg": data,
                            "timestamp": time.time(),
                            "retries": 0
                        }
                        print(f"    [追踪] 消息 [{msg_id}] 已发送给 {target}，等待 ACK...")
                    else:
                        # 3. 目标不在线：直接存入离线队列
                        if target not in offline_messages:
                            offline_messages[target] = []
                        offline_messages[target].append(data)
                        print(f"    [暂存] 用户 {target} 不在线，消息 [{msg_id}] 已存入离线队列")

            # --- 类型 3：心跳包 ---
            elif msg_type == MsgType.Heartbeat:
                pass # 生产环境中这里不打印，防止刷屏

            # --- 类型 4：已读回执 (ACK) ---
            elif msg_type == MsgType.Ack:
                msg_id = data.get("msgId")
                target = data.get("to") # ACK的发件人变成了收件人
                
                # 1. 服务端收到 ACK，将消息从监控池中安全移除
                with qos_lock:
                    if msg_id in unacked_messages:
                        del unacked_messages[msg_id]
                        print(f"[ACK] 质检通过: {sender} 确认收到消息 [{msg_id}]，解除追踪。")
                
                # 2. 把 ACK 转发给真正的发件人，让他的 UI 变绿
                with clients_lock:
                    target_conn = connected_clients.get(target)
                if target_conn:
                    send_msg_to_client(target_conn, data)

    except Exception as e:
        print(f"[-] {addr} 异常断开: {e}")
    finally:
        if current_user:
            with clients_lock:
                if current_user in connected_clients:
                    del connected_clients[current_user]
            print(f"[*] 用户下线: {current_user}")
        conn.close()

def start_im_server(host='127.0.0.1', port=8888):
    # 启动后台 QoS 质检员线程
    threading.Thread(target=qos_worker, daemon=True).start()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(10)
    print(f"[*] Python 增强型 IM 路由服务器已启动，监听 {host}:{port}...")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()

if __name__ == "__main__":
    start_im_server()