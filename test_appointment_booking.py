#!/usr/bin/env python3
"""
测试挂号功能的脚本
"""

import json
import socket
import time
import threading

def send_json_message(sock, message):
    """发送JSON消息到服务器"""
    json_str = json.dumps(message, ensure_ascii=False)
    message_bytes = json_str.encode('utf-8')
    length_bytes = len(message_bytes).to_bytes(4, byteorder='little')
    sock.sendall(length_bytes + message_bytes)

def receive_json_message(sock):
    """从服务器接收JSON消息"""
    try:
        # 读取消息长度
        length_bytes = sock.recv(4)
        if len(length_bytes) != 4:
            return None
        
        length = int.from_bytes(length_bytes, byteorder='little')
        
        # 读取消息内容
        message_bytes = b''
        while len(message_bytes) < length:
            chunk = sock.recv(length - len(message_bytes))
            if not chunk:
                return None
            message_bytes += chunk
        
        json_str = message_bytes.decode('utf-8')
        return json.loads(json_str)
    except Exception as e:
        print(f"接收消息时出错: {e}")
        return None

def test_appointment_booking():
    """测试挂号功能"""
    try:
        # 连接到服务器
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', 12345))
        print("已连接到服务器")
        
        # 1. 获取医生列表
        print("\n1. 获取医生列表...")
        get_doctors_msg = {
            "action": "get_all_doctors"
        }
        send_json_message(sock, get_doctors_msg)
        
        response = receive_json_message(sock)
        if response:
            print(f"医生列表响应: {json.dumps(response, ensure_ascii=False, indent=2)}")
        
        time.sleep(1)
        
        # 2. 测试挂号
        print("\n2. 测试挂号...")
        register_msg = {
            "action": "register_doctor",
            "doctor_name": "张医生",  # 使用默认医生
            "patientName": "patient1",  # 测试患者
            "doctorId": 1,  # 医生序号
            "uuid": f"test_register_{int(time.time())}"
        }
        send_json_message(sock, register_msg)
        
        response = receive_json_message(sock)
        if response:
            print(f"挂号响应: {json.dumps(response, ensure_ascii=False, indent=2)}")
        
        time.sleep(1)
        
        # 3. 查看预约列表
        print("\n3. 查看预约列表...")
        get_appointments_msg = {
            "action": "get_appointments_by_patient",
            "username": "patient1"
        }
        send_json_message(sock, get_appointments_msg)
        
        response = receive_json_message(sock)
        if response:
            print(f"预约列表响应: {json.dumps(response, ensure_ascii=False, indent=2)}")
        
        sock.close()
        print("\n测试完成")
        
    except Exception as e:
        print(f"测试时出错: {e}")

if __name__ == "__main__":
    print("开始测试挂号功能...")
    test_appointment_booking()
