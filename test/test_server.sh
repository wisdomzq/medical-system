#!/bin/bash

echo "停止可能存在的服务器进程..."
pkill -f "./server" || true
sleep 1

echo "启动服务器..."
cd /home/zephyr/medical-system/build/server
./server &
SERVER_PID=$!

echo "服务器PID: $SERVER_PID"
sleep 2

echo "检查服务器是否在运行..."
if ps -p $SERVER_PID > /dev/null; then
    echo "服务器启动成功"
else
    echo "服务器启动失败"
    exit 1
fi

echo "测试完成，停止服务器..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true

echo "测试结束"
