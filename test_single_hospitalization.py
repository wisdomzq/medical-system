#!/usr/bin/env python3
import socket
import json
import struct
import sys

def send_request(request):
    try:
        # 连接到服务器
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', 8888))
        print("Connected to server")
        
        # 将JSON转换为字节
        payload = json.dumps(request, ensure_ascii=False).encode('utf-8')
        
        # 构建协议头 (magic | version | type | payloadSize)
        magic = 0x1A2B3C4D
        version = 1
        message_type = 1  # JsonRequest
        payload_size = len(payload)
        
        # 打包头部（大端序）
        header = struct.pack('>IB H I', magic, version, message_type, payload_size)
        
        # 发送完整消息
        message = header + payload
        sock.send(message)
        print("Sent request:", request)
        
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
                return response
        else:
            print("Failed to receive response header")
            return None
            
        sock.close()
        
    except Exception as e:
        print(f"Error: {e}")
        return None

# 测试创建住院记录 - 使用最简单的数据
create_request = {
    "action": "create_hospitalization", 
    "data": {
        "patient_username": "88888",
        "doctor_username": "doctor1", 
        "admission_date": "2025-08-29",
        "ward": "内科病房",
        "bed_number": "101-01", 
        "diagnosis": "急性胃炎",
        "treatment_plan": "输液治疗",
        "daily_cost": 200.0,
        "notes": "测试记录"
    }
}

response = send_request(create_request)
