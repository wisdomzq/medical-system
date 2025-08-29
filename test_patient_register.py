#!/usr/bin/env python3
import socket
import json
import struct
import sys

def test_patient_register():
    try:
        # 连接到服务器
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', 8888))
        print("Connected to server")
        
        # 准备注册请求 - 测试新用户
        request = {
            "action": "register",
            "username": "99999",
            "password": "1",
            "role": "patient",
            "age": 30,
            "phone": "13900139000", 
            "address": "新测试地址456号"
        }
        
        # 将JSON转换为字节
        payload = json.dumps(request, ensure_ascii=False).encode('utf-8')
        
        # 构建协议头 (magic | version | type | payloadSize)
        # magic = 0x1A2B3C4D, version = 1, type = 1 (JsonRequest), payloadSize = len(payload)
        magic = 0x1A2B3C4D
        version = 1
        message_type = 1  # JsonRequest
        payload_size = len(payload)
        
        # 打包头部（大端序）
        header = struct.pack('>IB H I', magic, version, message_type, payload_size)
        
        # 发送完整消息
        message = header + payload
        sock.send(message)
        print("Sent register request:", request)
        
        # 接收响应头部
        header_data = sock.recv(11)  # 固定头部大小
        if len(header_data) == 11:
            recv_magic, recv_version, recv_type, recv_payload_size = struct.unpack('>IBH I', header_data)
            print(f"Response header: magic=0x{recv_magic:08x}, version={recv_version}, type={recv_type}, payload_size={recv_payload_size}")
            
            # 接收payload
            if recv_payload_size > 0:
                payload_data = sock.recv(recv_payload_size)
                response = json.loads(payload_data.decode('utf-8'))
                print("Received response:", response)
        else:
            print("Failed to receive response header")
            
        sock.close()
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    test_patient_register()
