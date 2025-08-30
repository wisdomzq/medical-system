# 聊天模块（业务对接说明）

> 面向业务模块同学的对接文档。基于现有自定义 JSON 请求-响应协议与 MessageRouter 路由范式，实现支持文本与文件消息（存元数据）的“长轮询聊天”。

- 适用端：现有 TCP 通信底座（Protocol::JsonRequest/JsonResponse），不改协议头与传输通道
- 兼容性：完全复用 MessageRouter 的 uuid/request_uuid 关联机制
- 目标能力：
  - 文本消息即时送达（长轮询）
  - 文件消息以“元数据”形态送达（不传二进制内容）
  - 历史补拉、已读/送达游标

---

## 1. 现有通信范式回顾

- 底座：Server 侧 `CommunicationServer + ClientHandler + MessageRouter`；Client 侧 `CommunicationClient + StreamFrameParser + ResponseDispatcher`
- 路由：`MessageRouter::requestReceived(QJsonObject payload)` 将每个 `JsonRequest` 广播到业务层；payload 内自动附带 `uuid`
- 业务回复：业务模块发出 `MessageRouter::onBusinessResponse(MessageType::JsonResponse, payload)`；payload 需包含 `request_uuid = <uuid>`，路由器据此回到对应连接与请求
- JSON 语义：各业务通过 `payload["action"]` 进行分发，示例可参考 `server/main.cpp` 与 `modules/*`

该范式天然支持“长时间不回应，待事件发生再回应”的模式（uuid -> handler 的映射已在路由器内维持），非常适合实现长轮询。

---

## 2. 长轮询整体方案

- 核心思路：
  - 客户端登录/上线后周期性发送 `action = "chat.poll"` 的 JSON 请求，声明“我准备好接收新消息”
  - 业务模块检测该用户是否有未下发消息：
    - 如果有，立即返回这些消息
    - 如果没有，挂起该请求（保存 `uuid` 与用户信息），直至有新消息到达或超时（例如 25~30s），再返回空结果
  - 客户端处理响应后，立刻再次发送 `chat.poll`，形成长轮询

- 交付语义：
  - 至少一次投递（At-least-once）：客户端以 `since_id` 作为下次游标，服务端返回 `id > since_id` 的消息；客户端幂等合并
  - 超时空响应保持连接活性，建议 `timeout_ms` 由客户端可配，服务端可设置上限（例如 60s）

---

## 3. JSON 接口定义

为与现有风格一致，统一使用 `action` 进行路由；所有响应均需回传 `request_uuid`。

### 3.1 拉取新消息（长轮询）

- 请求：
```json
{
  "action": "chat.poll",
  "user": "<username>",
  "since_id": 12345,         // 可选，上次已处理的最大 message_id；缺省为 0
  "conversations": ["peerA"],// 可选，限定会话/对端；缺省为全部
  "timeout_ms": 25000        // 可选，服务端最长挂起时长（服务端可裁剪）
}
```
- 响应（有新消息）：
```json
{
  "type": "chat_poll_response",
  "success": true,
  "events": [
    {
      "event_type": "message",
      "message": {
        "id": 12346,
        "conversation_id": "peerA", // 或群组/会话标识
        "from": "doctor_001",
        "to": "patient_007",
        "msg_type": "text",        // text | file
        "text": "早上好，复诊注意…",
        "server_time": "2025-08-30T06:25:04Z",
        "status": "delivered"       // delivered | read
      }
    }
  ],
  "next_since_id": 12346,
  "request_uuid": "<echo from request>"
}
```
- 响应（超时无新消息）：
```json
{
  "type": "chat_poll_response",
  "success": true,
  "events": [],
  "next_since_id": 12345,
  "request_uuid": "<echo>"
}
```

实现提示：无新消息时保存挂起上下文（user, uuid, 截止时间）并设 `QTimer` 超时后返回空数组。

### 3.2 发送消息（文本/文件元数据）

- 请求：
```json
{
  "action": "chat.send",
  "from": "doctor_001",
  "to": "patient_007",          // 或使用 conversation_id
  "conversation_id": "peerA",   // 可选，点对点可用对端作会话ID
  "msg_type": "text",           // text | file
  "text": "本周按时服药",
  "file": {                      // 当 msg_type = file 时必填（仅存元数据）
    "filename": "exam.pdf",
    "size": 345678,
    "mime": "application/pdf",
    "checksum": "sha256:…",
    "storage_url": "file:///var/app/chat/…/exam.pdf" // 或相对路径/对象存储URL
  }
}
```
- 响应：
```json
{
  "type": "chat_send_response",
  "success": true,
  "message_id": 12347,
  "server_time": "2025-08-30T06:25:31Z",
  "request_uuid": "<echo>"
}
```

服务端处理要点：
- 写入数据库（见下方表设计），为接收方触发“唤醒”逻辑：
  - 若接收方有挂起的 `chat.poll`，立刻用该 `uuid` 返回包含该消息的响应
  - 若没有挂起请求，则静默等待，接收方下次长轮询将补拉

### 3.3 标记已读/游标前进（可选但推荐）

- 请求：
```json
{
  "action": "chat.ack",
  "user": "patient_007",
  "last_read_id": 12347,
  "conversation_id": "peerA"
}
```
- 响应：
```json
{
  "type": "chat_ack_response",
  "success": true,
  "request_uuid": "<echo>"
}
```

### 3.4 历史消息查询（分页）

- 请求：
```json
{
  "action": "chat.history",
  "user": "patient_007",
  "conversation_id": "peerA",
  "before_id": 12340,      // 可选，向上翻页
  "limit": 50
}
```
- 响应：
```json
{
  "type": "chat_history_response",
  "success": true,
  "messages": [ /* 按 id 倒序或正序均可，需文档化 */ ],
  "request_uuid": "<echo>"
}
```

---

## 4. 数据库表设计（SQLite 示例）

以追加三张表为例，命名可按现有风格调整：

```sql
-- 会话（可选，点对点可不建）
CREATE TABLE IF NOT EXISTS chat_conversations (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  conversation_id TEXT UNIQUE,           -- 业务定义
  type TEXT,                             -- p2p | group
  created_at TEXT
);

-- 消息主表
CREATE TABLE IF NOT EXISTS chat_messages (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  conversation_id TEXT NOT NULL,
  sender TEXT NOT NULL,
  receiver TEXT NOT NULL,                -- p2p: 对端；群聊可放 groupId，或另建关联表
  msg_type TEXT NOT NULL,                -- text | file
  text TEXT,                             -- 文本消息体
  file_id INTEGER,                       -- 指向文件元数据
  status TEXT DEFAULT 'delivered',       -- delivered | read
  server_time TEXT
);
CREATE INDEX IF NOT EXISTS idx_chat_conv_id ON chat_messages(conversation_id, id);
CREATE INDEX IF NOT EXISTS idx_chat_receiver ON chat_messages(receiver, id);

-- 文件元数据（不存二进制）
CREATE TABLE IF NOT EXISTS chat_files (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  filename TEXT,
  size INTEGER,
  mime TEXT,
  checksum TEXT,
  storage_url TEXT,                      -- 本地路径或对象存储URL
  created_at TEXT
);
```

- 插入文本：只写 `chat_messages`（`msg_type='text'`, `text` 非空）
- 插入文件：先插 `chat_files`，得到 `file_id`，再插 `chat_messages`（`msg_type='file'`）
- 拉取：按 `id > since_id AND receiver = <user>`（或 conversation_id）
- 已读：更新 `chat_messages.status='read'`，可选维护每会话 `last_read_id`

注：二进制文件的上传与存储本方案不强制实现，当前仅管理元数据。如后续需要支持二进制，可增设 `chat.upload_*` 分块接口或复用现有静态资源存储。

---

## 5. 业务模块接入指南（Server 侧）

- 位置建议：`server/modules/chatmodule/` 新增 `chatmodule.{h,cpp}`，在 `server/main.cpp` 里类似其他模块注册并在 `action` 分支中分发到本模块
- 与路由器交互：
  - 监听 `MessageRouter::requestReceived`，筛选 `payload["action"]` 为 `chat.*`
  - 读取/写入 DB（复用 `DBManager`，按上表扩展方法）
  - 回复时务必：
    1) 设置 `response["type"]` 为对应的 `_response`
    2) 拷贝 `request_uuid`：若请求中有 `uuid`，则 `response["request_uuid"] = payload["uuid"]`
    3) 通过 `MessageRouter::onBusinessResponse(Protocol::MessageType::JsonResponse, response)` 发送

- 长轮询挂起：
  - 建立 `QHash<QString /*user*/, PendingPoll>`，`PendingPoll` 内含 `uuid`, `deadline(QDeadlineTimer 或 QDateTime)`, `filters`
  - `chat.poll` 处理流程：
    1) 先按 `since_id` 查询 DB，有数据则立即返回
    2) 无数据：记录 `PendingPoll{uuid,..}`，`QTimer::singleShot(timeout_ms, ...)` 到期返回空集
  - `chat.send` 插入成功后：
    - 查找接收方是否有 `PendingPoll`，若有则使用对应 `uuid` 直接回应并清理挂起

- 连接清理：若客户端断开，`MessageRouter` 会清理 `uuid->handler` 映射；业务侧也应在超时或回应后清理自身 `PendingPoll`

---

## 6. 客户端约定（UI/业务调用侧）

- 上线后立即发起一次 `chat.poll`；收到响应后立刻再次发起
- 每次 `chat.poll` 附带 `since_id`（本地保存的最大 `message_id`）
- 发送文本：`chat.send`，UI 立即本地显示为“已发送（待送达）”，待服务端响应成功后标记为“已送达”
- 发送文件：先完成文件的外部上传（若有），随后以 `msg_type=file` 携带元数据走 `chat.send`
- 已读同步（可选）：在消息展示后调用 `chat.ack` 推进游标

---

## 7. 错误与边界

- 包大小：协议上限 4MB，文件信息仅传元数据，文本内容建议 < 32KB
- 校验：
  - `chat.send` 当 `msg_type=text` 时需有 `text`；当 `msg_type=file` 时需有 `file{ filename,size,mime }`
  - 用户与权限校验可复用登录模块（根据 `user`/`from`/`to` 判断是否合法）
- 超时：长轮询 `timeout_ms` 建议 25~30s；服务端可统一设置上限
- 并发：同一用户多端在线时，允许存在多个挂起 poll，任一到新消息均需广播或按端策略送达

---

## 8. 示例代码片段（伪代码）

```cpp
// ChatModule::onRequest
void ChatModule::onRequest(const QJsonObject& payload) {
  const QString action = payload.value("action").toString();
  if (action == "chat.send") {
    // 1) 校验并入库（text/file 元数据） -> message_id
    // 2) 若 receiver 有挂起 poll：构造 chat_poll_response，设置 request_uuid = pending.uuid，onBusinessResponse(...)
    // 3) 无挂起则不立即回给对方，但要回给发送方 chat_send_response
  } else if (action == "chat.poll") {
    // 1) 以 since_id 查库；若有则立即返回 chat_poll_response
    // 2) 无则记录 pending，并启动超时定时器；定时器触发后回空 events
  } else if (action == "chat.ack") {
    // 更新已读/游标
  } else if (action == "chat.history") {
    // 分页查询历史
  }
}
```

---

## 9. 后续可选项

- 分块上传二进制：`chat.upload_init` / `chat.upload_chunk` / `chat.upload_complete`
- 已送达/已读回执事件作为 `events` 的一种
- 会话未读计数与红点同步
- 群聊与系统通知事件统一走 `events`

---

## 10. 对齐检查

- 复用 `MessageRouter` 与 `uuid/request_uuid` 路由，不改协议头
- 遵循 `action` 路由范式，与现有模块一致
- 数据仅 JSON，文件仅存元数据，满足协议的 4MB 限制
- 支持文本与文件消息需求，提供长轮询实时性

如需我方先行提供 `DBManager` 的增扩接口与 `ChatModule` 模板代码，可继续安排。
