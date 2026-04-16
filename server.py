import socket
import struct
import json

def start_echo_server(host='127.0.0.1', port=8888):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen(1)
    print(f"[*] Python Echo Server is listening on {host}:{port}...")

    conn, addr = server.accept()
    print(f"[+] Connected by {addr}")

    try:
        while True:
            # 1. 接收 4 字节的包头 (表示包体长度)
            header = conn.recv(4)
            if not header:
                break
            
            # 使用 struct 解包：'>I' 表示大端序 (网络字节序) 的 32 位无符号整数
            msg_length = struct.unpack('>I', header)[0]
            
            # 2. 根据长度接收 JSON 包体
            # 注意：实际生产环境中可能需要循环 recv 直到收满 msg_length，这里为了演示保持极简
            body = conn.recv(msg_length)
            json_str = body.decode('utf-8')
            print(f"[-] Received data: {json_str}")

            # 3. 原样送回 (Echo测试)
            # 打包方式：使用 struct.pack 封入 4 字节大端序长度，拼上字节流
            response_data = body
            response_header = struct.pack('>I', len(response_data))
            conn.sendall(response_header + response_data)

    except ConnectionResetError:
        print("[-] Client disconnected.")
    finally:
        conn.close()
        server.close()

if __name__ == "__main__":
    start_echo_server()