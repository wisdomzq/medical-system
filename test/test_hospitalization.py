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

def test_hospitalization_functions():
    print("=== 测试住院管理功能 ===\n")
    
    # 1. 创建住院记录
    print("1. 测试创建住院记录")
    create_request = {
        "action": "create_hospitalization",
        "data": {
            "patient_username": "88888",
            "doctor_username": "张医生",  # 假设有这个医生
            "admission_date": "2025-08-29",
            "ward": "内科病房",
            "bed_number": "101-01",
            "diagnosis": "急性胃炎",
            "treatment_plan": "输液治疗，观察病情",
            "daily_cost": 200.50,
            "status": "admitted",
            "notes": "患者病情稳定，需要继续观察"
        }
    }
    
    response = send_request(create_request)
    if response and response.get('success'):
        print("✅ 住院记录创建成功\n")
    else:
        print("❌ 住院记录创建失败\n")
    
    # 2. 获取患者的住院记录
    print("2. 测试获取患者住院记录")
    get_patient_request = {
        "action": "get_hospitalizations_by_patient",
        "patient_username": "88888"
    }
    
    response = send_request(get_patient_request)
    if response and response.get('success'):
        print("✅ 成功获取患者住院记录")
        print(f"   记录数量: {len(response.get('data', []))}")
    else:
        print("❌ 获取患者住院记录失败")
    
    print()
    
    # 3. 获取所有住院记录
    print("3. 测试获取所有住院记录")
    get_all_request = {
        "action": "get_all_hospitalizations"
    }
    
    response = send_request(get_all_request)
    if response and response.get('success'):
        print("✅ 成功获取所有住院记录")
        print(f"   记录数量: {len(response.get('data', []))}")
    else:
        print("❌ 获取所有住院记录失败")

if __name__ == "__main__":
    test_hospitalization_functions()
