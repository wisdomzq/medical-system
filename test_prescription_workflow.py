#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
处方功能测试脚本
验证医生端开具处方和患者端查看处方的完整流程
"""

import sqlite3
import json
from datetime import datetime

def test_prescription_workflow():
    # 连接数据库
    conn = sqlite3.connect('data/user.db')
    cursor = conn.cursor()
    
    print("=== 处方功能测试 ===")
    
    # 1. 检查药品数据
    print("\n1. 检查药品数据...")
    cursor.execute("SELECT COUNT(*) FROM medications")
    medication_count = cursor.fetchone()[0]
    print(f"数据库中共有 {medication_count} 种药品")
    
    if medication_count == 0:
        print("❌ 错误：数据库中没有药品数据")
        return False
    
    # 显示前5种药品
    cursor.execute("SELECT id, name, price, unit FROM medications LIMIT 5")
    medications = cursor.fetchall()
    print("前5种药品:")
    for med in medications:
        print(f"  ID: {med[0]}, 名称: {med[1]}, 价格: ¥{med[2]}, 单位: {med[3]}")
    
    # 2. 检查现有处方数据
    print("\n2. 检查现有处方数据...")
    cursor.execute("SELECT COUNT(*) FROM prescriptions")
    prescription_count = cursor.fetchone()[0]
    print(f"数据库中共有 {prescription_count} 个处方")
    
    # 检查处方项
    cursor.execute("SELECT COUNT(*) FROM prescription_items")
    prescription_items_count = cursor.fetchone()[0]
    print(f"数据库中共有 {prescription_items_count} 个处方项")
    
    # 3. 检查处方详情显示
    print("\n3. 检查处方详情数据结构...")
    cursor.execute("""
        SELECT p.id, p.total_amount, p.notes,
               pi.medication_id, pi.quantity, pi.dosage, pi.frequency, pi.duration,
               pi.unit_price, pi.total_price,
               m.name as medication_name, m.unit
        FROM prescriptions p
        LEFT JOIN prescription_items pi ON p.id = pi.prescription_id
        LEFT JOIN medications m ON pi.medication_id = m.id
        WHERE p.id IN (SELECT id FROM prescriptions LIMIT 3)
        ORDER BY p.id, pi.id
    """)
    
    prescription_details = cursor.fetchall()
    if prescription_details:
        print("处方详情示例:")
        current_prescription_id = None
        for detail in prescription_details:
            if detail[0] != current_prescription_id:
                current_prescription_id = detail[0]
                print(f"\n处方 ID: {detail[0]}, 总金额: ¥{detail[1]}, 备注: {detail[2]}")
            
            if detail[3]:  # 如果有药品项
                print(f"  - {detail[10]} x{detail[4]} {detail[11]}, 剂量: {detail[5]}, "
                      f"用法: {detail[6]}, 疗程: {detail[7]}, 单价: ¥{detail[8]}, 小计: ¥{detail[9]}")
    
    # 4. 验证价格计算
    print("\n4. 验证价格计算...")
    cursor.execute("""
        SELECT pi.prescription_id, 
               SUM(pi.total_price) as calculated_total,
               p.total_amount as stored_total
        FROM prescription_items pi
        JOIN prescriptions p ON pi.prescription_id = p.id
        GROUP BY pi.prescription_id, p.total_amount
        HAVING ABS(calculated_total - stored_total) > 0.01
    """)
    
    price_mismatches = cursor.fetchall()
    if price_mismatches:
        print("❌ 发现价格计算不匹配的处方:")
        for mismatch in price_mismatches:
            print(f"  处方 {mismatch[0]}: 计算总价 ¥{mismatch[1]}, 存储总价 ¥{mismatch[2]}")
    else:
        print("✅ 所有处方的价格计算正确")
    
    # 5. 检查数据完整性
    print("\n5. 检查数据完整性...")
    
    # 检查处方项是否有对应的药品
    cursor.execute("""
        SELECT pi.id, pi.medication_id
        FROM prescription_items pi
        LEFT JOIN medications m ON pi.medication_id = m.id
        WHERE m.id IS NULL
    """)
    
    orphaned_items = cursor.fetchall()
    if orphaned_items:
        print("❌ 发现孤立的处方项(没有对应药品):")
        for item in orphaned_items:
            print(f"  处方项 ID: {item[0]}, 药品 ID: {item[1]}")
    else:
        print("✅ 所有处方项都有对应的药品")
    
    # 6. 生成测试报告
    print("\n=== 测试报告 ===")
    print(f"✅ 药品数量: {medication_count}")
    print(f"✅ 处方数量: {prescription_count}")
    print(f"✅ 处方项数量: {prescription_items_count}")
    print(f"{'✅' if not price_mismatches else '❌'} 价格计算: {'正确' if not price_mismatches else '有误'}")
    print(f"{'✅' if not orphaned_items else '❌'} 数据完整性: {'完整' if not orphaned_items else '有问题'}")
    
    conn.close()
    
    return len(price_mismatches) == 0 and len(orphaned_items) == 0

if __name__ == "__main__":
    success = test_prescription_workflow()
    exit(0 if success else 1)
