#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
处方功能演示脚本
模拟医生开具处方的完整流程
"""

import sqlite3
import json
from datetime import datetime

def create_demo_prescription():
    """创建一个演示处方"""
    conn = sqlite3.connect('data/user.db')
    cursor = conn.cursor()
    
    print("=== 医生开具处方演示 ===")
    
    # 1. 医生选择药品
    print("\n1. 医生选择药品...")
    
    # 模拟医生选择3种药品
    selected_medications = [
        {
            "medication_id": 1,  # 阿司匹林肠溶片
            "quantity": 2,
            "dosage": "100mg",
            "frequency": "每日三次",
            "duration": "7天",
            "instructions": "饭后服用"
        },
        {
            "medication_id": 4,  # 阿莫西林胶囊
            "quantity": 3,
            "dosage": "0.25g",
            "frequency": "每日两次", 
            "duration": "5天",
            "instructions": "空腹服用"
        },
        {
            "medication_id": 6,  # 奥美拉唑肠溶胶囊
            "quantity": 1,
            "dosage": "20mg",
            "frequency": "每日一次",
            "duration": "14天", 
            "instructions": "早餐前服用"
        }
    ]
    
    # 2. 获取药品详细信息并计算价格
    print("\n2. 计算处方详情...")
    total_amount = 0.0
    prescription_items = []
    
    for item in selected_medications:
        # 获取药品信息
        cursor.execute("SELECT name, price, unit FROM medications WHERE id = ?", 
                      (item["medication_id"],))
        med_info = cursor.fetchone()
        
        if med_info:
            name, unit_price, unit = med_info
            quantity = item["quantity"]
            total_price = unit_price * quantity
            total_amount += total_price
            
            prescription_item = {
                **item,
                "medication_name": name,
                "unit_price": unit_price,
                "total_price": total_price,
                "unit": unit
            }
            prescription_items.append(prescription_item)
            
            print(f"  - {name} x{quantity} {unit}")
            print(f"    剂量: {item['dosage']}, 用法: {item['frequency']}, 疗程: {item['duration']}")
            print(f"    单价: ¥{unit_price}, 小计: ¥{total_price}")
    
    print(f"\n处方总金额: ¥{total_amount}")
    
    # 3. 创建处方记录
    print("\n3. 创建处方记录...")
    
    # 插入处方主记录
    prescription_data = {
        "record_id": 1,  # 假设的病历记录ID
        "patient_username": "patient001",
        "doctor_username": "doctor001", 
        "prescription_date": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "total_amount": total_amount,
        "status": "pending",
        "notes": "新处方演示 - 按时服药，如有不适请及时就医"
    }
    
    cursor.execute("""
        INSERT INTO prescriptions (record_id, patient_username, doctor_username, 
                                 prescription_date, total_amount, status, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    """, (
        prescription_data["record_id"],
        prescription_data["patient_username"], 
        prescription_data["doctor_username"],
        prescription_data["prescription_date"],
        prescription_data["total_amount"],
        prescription_data["status"],
        prescription_data["notes"]
    ))
    
    prescription_id = cursor.lastrowid
    print(f"处方记录已创建，ID: {prescription_id}")
    
    # 4. 添加处方项
    print("\n4. 添加处方项...")
    
    for item in prescription_items:
        cursor.execute("""
            INSERT INTO prescription_items (prescription_id, medication_id, quantity,
                                          dosage, frequency, duration, instructions,
                                          unit_price, total_price)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            prescription_id,
            item["medication_id"],
            item["quantity"],
            item["dosage"],
            item["frequency"], 
            item["duration"],
            item["instructions"],
            item["unit_price"],
            item["total_price"]
        ))
        
        print(f"  ✅ 添加药品: {item['medication_name']}")
    
    conn.commit()
    
    # 5. 验证创建的处方
    print("\n5. 验证创建的处方...")
    
    cursor.execute("""
        SELECT p.id, p.total_amount, p.notes,
               pi.medication_id, pi.quantity, pi.dosage, pi.frequency, pi.duration,
               pi.unit_price, pi.total_price,
               m.name as medication_name, m.unit
        FROM prescriptions p
        LEFT JOIN prescription_items pi ON p.id = pi.prescription_id
        LEFT JOIN medications m ON pi.medication_id = m.id
        WHERE p.id = ?
        ORDER BY pi.id
    """, (prescription_id,))
    
    results = cursor.fetchall()
    
    if results:
        print(f"\n✅ 处方验证成功 (ID: {results[0][0]})")
        print(f"总金额: ¥{results[0][1]}")
        print(f"备注: {results[0][2]}")
        print("药品清单:")
        
        for result in results:
            if result[3]:  # 如果有药品项
                print(f"  - {result[10]} x{result[4]} {result[11]}")
                print(f"    剂量: {result[5]}, 用法: {result[6]}, 疗程: {result[7]}")
                print(f"    单价: ¥{result[8]}, 小计: ¥{result[9]}")
    
    conn.close()
    
    print(f"\n=== 处方创建完成 ===")
    print(f"新处方ID: {prescription_id}")
    print("患者现在可以在客户端查看此处方详情")
    
    return prescription_id

if __name__ == "__main__":
    create_demo_prescription()
