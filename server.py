import socket
import struct
import json
import threading

# 全局字典，用于存储所有在线用户的映射关系：{"username": socket_connection}
connected_clients = {}
# 线程锁，防止多线程同时读写字典导致崩溃
clients_lock = threading.Lock()

def handle_client(conn, addr):
    print(f"[+] 接受来自 {addr} 的新连接")
    current_user = None
    
    try:
        while True:
            # 1. 读取 4 字节包头
            header = conn.recv(4)
            if not header:
                break
            msg_length = struct.unpack('>I', header)[0]
            
            # 2. 读取 JSON 包体
            body = conn.recv(msg_length)
            data = json.loads(body.decode('utf-8'))
            
            msg_type = data.get("type")
            sender = data.get("from")

            # --- 业务逻辑分发 ---
            
            # 类型 1：处理登录请求
            if msg_type == 1:
                with clients_lock:
                    connected_clients[sender] = conn
                current_user = sender
                print(f"[*] 用户上线: {sender}. 当前在线人数: {len(connected_clients)}")

            # 类型 2：处理聊天消息转发
            elif msg_type == 2:
                target = data.get("to")
                print(f"[-] 收到转发请求: {sender} -> {target}, 内容: {data.get('msg')}")
                
                with clients_lock:
                    target_conn = connected_clients.get(target)
                
                if target_conn:
                    # 如果目标在线，原封不动地把收到的二进制数据包（头+体）转发过去
                    response_header = struct.pack('>I', len(body))
                    target_conn.sendall(response_header + body)
                    print(f"    [成功] 已转发给 {target}")
                else:
                    # 目标不在线 (这里未来可以结合你的“离线消息落库”需求进行扩展)
                    print(f"    [失败] 用户 {target} 不在线")
            elif msg_type == 3:
                print(f"[♥] 扑通! 收到 {sender} 的心跳包，连接正常活跃中...")
                
            elif msg_type == 4:
                msg_id = data.get("msgId")
                target = data.get("to")
                # 1. 服务端核心逻辑：确认消息送达！
                print(f"[ACK] 质检通过: 用户 {sender} 已成功接收消息 [{msg_id}]")
                # （注：未来如果加了数据库，这里就是执行 DELETE 离线消息的操作）

                with clients_lock:
                    target_conn = connected_clients.get(target)
                
                if target_conn:
                    response_header = struct.pack('>I', len(body))
                    target_conn.sendall(response_header + body)
                    
    except ConnectionResetError:
        print(f"[-] {addr} 异常断开连接。")
    except Exception as e:
        print(f"[-] 处理 {addr} 时发生错误: {e}")
    finally:
        # 客户端断开时，将其从全局字典中清理掉
        if current_user:
            with clients_lock:
                if current_user in connected_clients:
                    del connected_clients[current_user]
            print(f"[*] 用户下线: {current_user}. 当前在线人数: {len(connected_clients)}")
        conn.close()

def start_im_server(host='127.0.0.1', port=8888):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen(10) # 支持最大等待连接数
    print(f"[*] Python IM 路由服务器已启动，监听 {host}:{port}...")

    # 主循环：不断接受新连接并分配线程
    while True:
        conn, addr = server.accept()
        # 为每一个接入的客户端启动一个独立的线程
        thread = threading.Thread(target=handle_client, args=(conn, addr))
        # 设置为守护线程，主程序退出时子线程自动结束
        thread.daemon = True 
        thread.start()

if __name__ == "__main__":
    start_im_server()