# BetterCommunicationModule 开发文档（Qt 5）

> 一个基于 Qt 5 的轻量自定义二进制协议通信库，带心跳与自动重连，内置客户端/服务端示例，可作为业务系统通信底座复用与扩展。

- 代码位置：`src/client`, `src/server`, `src/protocol.h`
- 示例程序：`examples/server_main.cpp`, `examples/client_cli.cpp`
- 构建系统：CMake（Qt 5.15 及以上推荐）

---

## 目录

- 背景与定位
- 架构总览
- 自定义协议说明
- 客户端组件
- 服务端组件
- 线程模型与并发
- 数据流与时序
- 扩展与二次开发
- 构建与运行
- 错误处理与故障排查
- 性能与安全建议
- API 参考（简版）
- 附录：报文头布局与常量

---

## 背景与定位

- 目标：提供“稳定可控”的 TCP 通信底座，统一封装连接管理、分包粘包、心跳保活、自动重连与报文分派，业务仅关注 JSON 语义。
- 适用：内网/局域网应用、诊疗/设备管理系统、微服务内通信桥接等。
- 特点：
  - 自定义二进制固定头 + JSON 有效载荷。
  - 大端序，跨平台一致。
  - 心跳 PING/PONG 与超时处理；客户端指数退避重连。
  - 线程隔离（每连接一线程），调度器单例集中路由。

---

## 架构总览

- Client 侧
  - `CommunicationClient` 负责连接、心跳、发送 JSON、接收数据；
  - `StreamFrameParser` 增量解析流，按协议切分完整帧；
  - `ResponseDispatcher` 根据头部类型分发到 `jsonReceived`、`errorOccurred`、`heartbeatPong`。
- Server 侧
  - `CommunicationServer` 监听端口、为每个新连接创建一个 `ClientHandler` 所在线程；
  - `ClientHandler` 在连接线程中读取、解析协议，发出 `requestReady`；
  - `RequestDispatcher`（单例，通常在主线程）按消息类型路由，生成响应并通过信号回发给对应的 `ClientHandler`。

代码组织：

- `src/protocol.h` 协议常量、头部布局、打包/解析 JSON 辅助；
- `src/client/*` 客户端实现；
- `src/server/*` 服务端实现；
- `examples/*` 可执行示例（ServerApp/ClientCLI）。

---

## 自定义协议说明（位于 `src/protocol.h`）

- 固定头部（大端序）：
  - `magic(4)` 固定魔数 `0x1A2B3C4D`
  - `version(1)` 协议版本，当前 `1`
  - `type(2)` `Protocol::MessageType`
  - `payloadSize(4)` 有效载荷字节数
- 有效载荷（可为空）：
  - 约定为 `QJsonObject` 的紧凑 JSON 字节串（`QJsonDocument::Compact`）。
- 消息类型 `enum class MessageType : quint16`
  - `JsonRequest = 1`
  - `JsonResponse = 2`
  - `ErrorResponse = 3`
  - `HeartbeatPing = 4`
  - `HeartbeatPong = 5`
- 最大包长：`MAX_PACKET_SIZE = 4MB`（含 payload，不含操作系统层分片）
- 心跳：`HEARTBEAT_INTERVAL_MS = 30000`，超时 `HEARTBEAT_TIMEOUT_MS = 5000`

实用函数：
- `QByteArray pack(MessageType, const QByteArray& payload)` 打包出站帧；
- `QByteArray toJsonPayload(const QJsonObject&)` JSON → 紧凑字节；
- `QJsonObject fromJsonPayload(const QByteArray&)` 解析 JSON，失败时返回 `{ error, detail }`。

---

## 客户端组件（`src/client`）

### CommunicationClient

职责：
- 建立 TCP 连接、自动重连（指数退避，1s 起，最大 60s）；
- 定时发送心跳 `HeartbeatPing`，等待 `HeartbeatPong`，超时主动断开以触发重连；
- 发送 JSON 请求（封装为 `JsonRequest`）；
- 读取字节流，交给解析器与分发器。

关键 API：
- `void connectToServer(const QString& host, quint16 port)` 连接或重连目标；
- `void sendJson(const QJsonObject& obj)` 发送业务 JSON 请求；
- 信号：
  - `connected() / disconnected()` 连接状态；
  - `jsonReceived(const QJsonObject&)` 收到 `JsonResponse`；
  - `errorOccurred(int code, const QString& message)` 收到 `ErrorResponse`。

内部组成：
- `QTcpSocket m_socket` 原生套接字；
- `StreamFrameParser m_parser`：增量拆帧，处理半包/粘包；
- `ResponseDispatcher m_dispatcher`：按 `Header.type` 路由。

### StreamFrameParser

- 将任意长度的到达字节追加到 `m_buffer`；
- 若可解析完整固定头，检查 `magic`/`version`/`payloadSize`；
- `payload` 不足则继续等待；足够则发出 `frameReady(header, payload)`；
- 发现非法头或超长 payload → 发出 `protocolError` 并清空缓冲。

### ResponseDispatcher

- 根据 `Header.type` 分发：
  - `JsonResponse` → `jsonResponse(obj)`；
  - `ErrorResponse` → `errorResponse(code, msg)`；
  - `HeartbeatPong` → `heartbeatPong()`。

---

## 服务端组件（`src/server`）

### CommunicationServer（监听器）

- 继承 `QTcpServer`，覆盖 `incomingConnection`；
- 每个连接创建一个 `QThread` 和一个 `ClientHandler` 实例，并把 handler 移动到该线程；
- 将 `ClientHandler::requestReady`（发自连接线程）转发至 `RequestDispatcher::onRequestReady`（队列连接）；
- 监听 `RequestDispatcher::responseReady`，将发往本 handler 的消息转发到 `handler->sendMessage(...)`。

### ClientHandler（每连接一实例，连接线程执行）

- 管理该连接的 `QTcpSocket`；
- 自行维护解析状态机，确保按协议读取固定头与 payload；
- 解析出完整 JSON 后，发出 `requestReady(this, header, json)`；
- 断开时 `deleteLater()` 自清理。

### RequestDispatcher（单例，集中路由）

- 入口：`onRequestReady(ClientHandler* sender, Header header, QJsonObject payload)`；
- 缺省策略：
  - `JsonRequest` → 回声并附加 `serverTime` 字段（示例逻辑）；
  - `HeartbeatPing` → 立即回发 `HeartbeatPong`；
  - 其他类型 → 回发 `ErrorResponse{ 400, "Unsupported message type" }`。
- 出站：`responseReady(targetHandler, type, payload)`，由对应连接线程调用 `handler->sendMessage(...)` 真正写回。

---

## 线程模型与并发

- 监听线程：创建连接、创建/销毁连接线程；
- 连接线程：`ClientHandler` 的读写与解析；
- 调度线程：`RequestDispatcher` 单例，通常位于主线程；
- 跨线程通过 `Qt::QueuedConnection` 发送信号，确保线程安全；
- 每个连接独享线程，避免单连接阻塞影响其他连接；如需高并发可切换到线程池或异步 I/O（可作为后续演进）。

---

## 数据流与时序（文字版）

1) Client 启动：`connectToServer(host, port)` → `QTcpSocket::connectToHost`。
2) 连接成功：启动心跳定时器，每 `30s` 发送 `HeartbeatPing`，并开启 `5s` pong 超时计时；
3) Client 发送业务：`sendJson({...})` → `JsonRequest`；
4) Server 读流：`ClientHandler` 解析出一帧 → `requestReady(this, header, json)`；
5) Dispatcher 路由：生成响应并 `responseReady(target, type, payload)`；
6) Handler 发送：`pack(type, payload)` → `QTcpSocket::write`；
7) Client 收到：`StreamFrameParser` → `ResponseDispatcher` → `jsonReceived/errorOccurred/heartbeatPong`；
8) 心跳超时：Client 未收到 pong → `abort()` 断开并指数退避重连。

---

## 扩展与二次开发

推荐按“消息类型 + JSON 语义”两层扩展：

- 方式 A：保留 `MessageType::JsonRequest/JsonResponse`，在 JSON 内部用 `action` 字段分发。
  - 优点：兼容性强，类型数量少；
  - 服务端只需在 `RequestDispatcher::handleJson(...)` 中基于 `payload["action"]` 路由。

- 方式 B：新增或重载 `MessageType`（需要同时修改 `src/protocol.h` 和双方逻辑）。
  - 优点：在协议层面区分更清晰，能实现非 JSON 负载（如二进制）；
  - 需要更新 `StreamFrameParser/ClientHandler/ResponseDispatcher/RequestDispatcher` 对新类型的处理。

示例：按 action 路由（服务端）

```cpp
// src/server/requestdispatcher.cpp (示意)
void RequestDispatcher::handleJson(ClientHandler* sender, const Header&, const QJsonObject& payload) {
    const QString action = payload.value("action").toString();
    if (action == "echo") {
        QJsonObject resp = payload;
        resp["serverTime"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        emit responseReady(sender, MessageType::JsonResponse, resp);
        return;
    }
    if (action == "sum") {
        int a = payload.value("a").toInt();
        int b = payload.value("b").toInt();
        emit responseReady(sender, MessageType::JsonResponse, QJsonObject{{"sum", a + b}});
        return;
    }
    emit responseReady(sender, MessageType::ErrorResponse, QJsonObject{{"errorCode", 404}, {"errorMessage", "Unknown action"}});
}
```

客户端发送：

```cpp
client.sendJson(QJsonObject{ {"action", "sum"}, {"a", 7}, {"b", 35} });
```

注意事项：
- 严格校验字段与类型，避免崩溃；
- 控制 payload 大小与复杂度，防止超时与内存压力；
- 如需鉴权，可在首次 `JsonRequest` 中进行 token/签名校验，并在 `ClientHandler` 维护认证状态。

---

## 构建与运行

前置：
- 构建工具：CMake（3.16+ 建议）、GNU Make、g++；
- Qt 版本：5.15.x（与代码中 `QDataStream::Qt_5_15` 对齐），安装 `Qt5Core/Qt5Network` 开发包；

步骤：

```bash
# 1) 生成构建目录
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2) 编译
cmake --build build -j

# 3) 运行服务端（监听 8888）
./build/ServerApp

# 4) 运行客户端（默认 127.0.0.1:8888，可传 host/port）
./build/ClientCLI 127.0.0.1 8888
```

期望输出：
- 服务端打印“Server listening on …”与连接/请求日志；
- 客户端打印“Client connected”与 JSON 回显结果，随后定期 PING/PONG。

---

## 错误处理与故障排查

常见错误源与行为：
- 协议头非法（`magic/version` 不匹配）：
  - 客户端：`StreamFrameParser::protocolError` → 主动 `abort()`；
  - 服务端：`ClientHandler` 发现非法头 → `disconnectFromHost()`；
- payload 超过上限：同上，立即断开连接；
- JSON 解析失败：`fromJsonPayload` 返回 `{ error, detail }` 并打印告警；
- 心跳超时：客户端 5s 未收 `PONG` → `abort()` 并指数退避重连；
- 未支持的消息类型：服务端回 `ErrorResponse{400, "Unsupported message type"}`。

排查建议：
- 打开 Qt 日志并关注 `[Client]`, `[Handler]`, `[Dispatcher]` 前缀；
- 确认双方 Qt 版本一致（影响 `QDataStream` 序列化兼容性设置）；
- 使用 `tcpdump/wireshark` 验证字节序与长度字段；
- 检查心跳参数是否被业务长耗时阻塞（避免在连接线程执行重 CPU/IO 操作）。

---

## 性能与安全建议

- 性能
  - 零拷贝：`QByteArray` 共享数据语义减少不必要拷贝；
  - 分包处理：支持粘包/半包；
  - 并发：默认每连接一线程，单机连接数高时建议切换为线程池或事件驱动模型；
  - 大包：慎用接近 4MB 的包，避免 UI 卡顿或 GC 压力。

- 安全
  - 输入校验：严格校验 `payloadSize` 与 JSON 字段；
  - 限流与并发：增加连接/速率限制防 DoS；
  - 加密：如需加密与认证，建议将 `QTcpSocket` 替换为 `QSslSocket` 并启用双向 TLS；
  - 鉴权：在 `RequestDispatcher` 首次请求时校验 token/签名，未通过则断开。

---

## API 参考（简版）

### 命名空间 `Protocol`（`src/protocol.h`）
- 常量：`MAGIC`, `VERSION`, `SERVER_PORT`, `MAX_PACKET_SIZE`, `HEARTBEAT_INTERVAL_MS`, `HEARTBEAT_TIMEOUT_MS`, `FIXED_HEADER_SIZE`
- 类型：`struct Header { quint32 magic; quint8 version; MessageType type; quint32 payloadSize; }`
- 枚举：`enum class MessageType { JsonRequest=1, JsonResponse=2, ErrorResponse=3, HeartbeatPing=4, HeartbeatPong=5 }`
- 函数：`pack(type, payload)`, `toJsonPayload(obj)`, `fromJsonPayload(bytes)`

### 客户端
- `class CommunicationClient : QObject`
  - `void connectToServer(const QString&, quint16)`
  - `void sendJson(const QJsonObject&)`
  - 信号：`connected()`, `disconnected()`, `jsonReceived(obj)`, `errorOccurred(code, msg)`
- `class StreamFrameParser : QObject`
  - `void append(const QByteArray&)`, `void reset()`
  - 信号：`frameReady(Header, QByteArray)`, `protocolError(msg)`
- `class ResponseDispatcher : QObject`
  - 槽：`onFrame(Header, QByteArray)`
  - 信号：`jsonResponse(obj)`, `errorResponse(code, msg)`, `heartbeatPong()`

### 服务端
- `class CommunicationServer : QTcpServer`
  - `void incomingConnection(qintptr) override`
- `class ClientHandler : QObject`
  - `void initialize(qintptr socketDescriptor)`
  - 槽：`void sendMessage(MessageType, const QJsonObject&)`
  - 信号：`void requestReady(ClientHandler*, Header, QJsonObject)`
- `class RequestDispatcher : QObject`（单例）
  - `static RequestDispatcher& instance()`
  - 槽：`void onRequestReady(ClientHandler*, Header, QJsonObject)`
  - 信号：`void responseReady(ClientHandler* target, MessageType, QJsonObject)`

---

## 附录：报文头布局与常量

```
// 大端序 Big-Endian
+----------------+----------+----------+--------------+
| magic (4B)     | ver (1B) | type(2B) | payload(4B)  |
+----------------+----------+----------+--------------+
| 0x1A2B3C4D     |    0x01  |   see    | size in byte |
|                |          | Message  |              |
|                |          |  Type    |              |
+----------------+----------+----------+--------------+
```

默认端口：`Protocol::SERVER_PORT = 8888`

最大包长：`Protocol::MAX_PACKET_SIZE = 4 * 1024 * 1024`

心跳：
- 间隔：`Protocol::HEARTBEAT_INTERVAL_MS = 30000`
- 超时：`Protocol::HEARTBEAT_TIMEOUT_MS = 5000`

---

## 维护与后续演进建议

- 将 `RequestDispatcher` 的 JSON 路由拆成可注册式（action → handler）以便插件化；
- 引入协议版本协商与向后兼容策略；
- 支持二进制流类型（文件传输/分块）与断点续传；
- 抽象线程模型，支持线程池或协程化后端；
- 增加单元测试（帧解析、心跳、重连、路由）与基准测试。

---

如需更细粒度 API 文档，可在代码处补充 Doxygen 注释并生成 HTML 文档；本 README 将随代码变更持续更新。
