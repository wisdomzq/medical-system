#!/usr/bin/python3
import socket
import json
import time
import struct

def test_registration():
    # 连接到服务器
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(('127.0.0.1', 8888))
        print("成功连接到服务器")

        # 测试病人注册
        register_data = {
            "action": "register",
            "username": "66666",
            "password": "1",
            "role": "patient",
            "age": 25,
            "phone": "12345678901",
            "address": "测试地址"
        }
        
        # 创建JSON payload
        payload = json.dumps(register_data).encode('utf-8')
        
        # 创建协议头：magic(4) | version(1) | type(2) | payloadSize(4)
        magic = 0x1A2B3C4D
        version = 1  
        message_type = 1  # JsonRequest
        payload_size = len(payload)
        
        # 使用大端序打包头部
        header = struct.pack('>IbHI', magic, version, message_type, payload_size)
        
        # 发送头部和payload
        sock.sendall(header + payload)
        print(f"发送注册请求: {register_data}")

        # 接收响应头部
        response_header = sock.recv(11)  # magic(4) + version(1) + type(2) + payloadSize(4)
        if len(response_header) < 11:
            print("接收响应头部失败")
            return
        
        # 解析响应头部
        resp_magic, resp_version, resp_type, resp_payload_size = struct.unpack('>IbHI', response_header)
        print(f"响应头: magic={hex(resp_magic)}, version={resp_version}, type={resp_type}, payload_size={resp_payload_size}")
        
        # 接收响应payload
        response_payload = sock.recv(resp_payload_size)
        response = json.loads(response_payload.decode('utf-8'))
        print(f"收到响应: {response}")
        
        # 测试重复注册
        print("\n测试重复注册...")
        header = struct.pack('>IbHI', magic, version, message_type, payload_size)
        sock.sendall(header + payload)
        print(f"再次发送注册请求: {register_data}")

        # 接收第二次响应
        response_header2 = sock.recv(11)
        if len(response_header2) >= 11:
            resp_magic2, resp_version2, resp_type2, resp_payload_size2 = struct.unpack('>IbHI', response_header2)
            response_payload2 = sock.recv(resp_payload_size2)
            response2 = json.loads(response_payload2.decode('utf-8'))
            print(f"收到第二次响应: {response2}")
        
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()
    finally:
        sock.close()

if __name__ == "__main__":
    test_registration()
