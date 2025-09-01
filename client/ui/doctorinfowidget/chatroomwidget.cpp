#include "chatroomwidget.h"
#include "core/services/chatservice.h"
#include "core/network/communicationclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

ChatRoomWidget::ChatRoomWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent), m_doctor(doctorName), m_client(client) {
    auto* root = new QHBoxLayout(this);
    // 左侧：会话列表
    auto* left = new QVBoxLayout();
    left->addWidget(new QLabel(QString("医生聊天室 - %1").arg(m_doctor)));
    m_convList = new QListWidget(this);
    left->addWidget(m_convList, 1);
    auto *addLine = new QHBoxLayout();
    m_peerEdit = new QLineEdit(this);
    m_peerEdit->setPlaceholderText("输入患者用户名，例如 alice");
    m_addPeerBtn = new QPushButton("添加会话", this);
    addLine->addWidget(m_peerEdit,1);
    addLine->addWidget(m_addPeerBtn);
    left->addLayout(addLine);

    // 右侧：消息区
    auto* right = new QVBoxLayout();
    auto ctrl = new QHBoxLayout();
    m_loadMoreBtn = new QPushButton("加载更多历史", this);
    ctrl->addWidget(m_loadMoreBtn);
    ctrl->addStretch();
    right->addLayout(ctrl);

    m_list = new QListWidget(this);
    right->addWidget(m_list, 1);

    auto bottom = new QHBoxLayout();
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("输入消息，回车发送");
    m_sendBtn = new QPushButton("发送", this);
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_sendBtn);
    right->addLayout(bottom);

    root->addLayout(left, 1);
    root->addLayout(right, 2);

    setLayout(root);

    m_chat = new ChatService(m_client, m_doctor, this);
    connect(m_sendBtn, &QPushButton::clicked, this, &ChatRoomWidget::sendClicked);
    connect(m_input, &QLineEdit::returnPressed, this, &ChatRoomWidget::sendClicked);
    connect(m_addPeerBtn, &QPushButton::clicked, this, &ChatRoomWidget::addPeer);
    connect(m_convList, &QListWidget::currentRowChanged, this, &ChatRoomWidget::onPeerChanged);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &ChatRoomWidget::loadMore);
    connect(m_chat, &ChatService::historyReceived, this, &ChatRoomWidget::onHistory);
    connect(m_chat, &ChatService::eventsReceived, this, &ChatRoomWidget::onEvents);
    connect(m_chat, &ChatService::sendMessageResult, this, [this](bool ok, const QJsonObject& data){
        if (!ok) return;
        // 发送成功后立即刷新列表（等待服务端广播到本用户的poll也可以，但这里直接追加一个本地回显）
        QJsonObject m; m["id"] = data.value("id"); m["sender_username"] = m_doctor; m["text_content"] = m_input->text();
        // 注意：m_input已清空，这里不再使用
        // 真实展示依赖 poll 返回的完整记录；此回显仅确保即时反馈
        // appendMessage(m); // 可选：如需即时回显可解注释
    });
    connect(m_chat, &ChatService::recentContactsReceived, this, [this](const QJsonArray& contacts){
        m_convList->clear();
        for (const auto &v : contacts) m_convList->addItem(v.toObject().value("username").toString());
        if (m_convList->count()>0) m_convList->setCurrentRow(0);
    });
    // 自动初始化会话列表
    m_chat->recentContacts(20);

    // 启动长轮询（链式触发）
    schedulePoll();
}

void ChatRoomWidget::sendClicked() {
    if (m_currentPeer.isEmpty()) return;
    if (!m_input->text().isEmpty()) {
        m_chat->sendText(m_doctor, m_currentPeer, m_input->text());
        m_input->clear();
    }
}

void ChatRoomWidget::addPeer() {
    const QString peer = m_peerEdit->text().trimmed();
    if (peer.isEmpty()) return;
    // 如果已存在则选择，否则新增
    for (int i=0;i<m_convList->count();++i) {
        if (m_convList->item(i)->text()==peer){ m_convList->setCurrentRow(i); return; }
    }
    m_convList->addItem(peer);
    m_convList->setCurrentRow(m_convList->count()-1);
}

void ChatRoomWidget::onPeerChanged() {
    auto *item = m_convList->currentItem();
    m_list->clear();
    if (!item) { m_currentPeer.clear(); return; }
    m_currentPeer = item->text();
    m_earliestByPeer[m_currentPeer] = 0; // 未知最早id
    // 拉取最近一页（beforeId=0 表示从最新开始向后取）
    m_chat->getHistory(m_doctor, m_currentPeer, 0, 20);
}

void ChatRoomWidget::appendMessage(const QJsonObject& m) {
    // 仅显示当前会话（m_currentPeer）相关的消息
    const QString doctor = m.value("doctor_username").toString();
    const QString patient = m.value("patient_username").toString();
    if (m_currentPeer.isEmpty() || (doctor!=m_doctor && patient!=m_currentPeer) || (doctor==m_doctor && patient!=m_currentPeer)) {
        if (!(doctor==m_doctor && patient==m_currentPeer)) return;
    }
    const QString sender = m.value("sender_username").toString(m.value("sender").toString());
    const QString text = m.value("text_content").toString();
    const int id = m.value("id").toInt();
    // 加上角色标识
    QString role = (sender==m_doctor)? "医生" : "患者";
    m_list->addItem(QString("#%1 [%2][%3]: %4").arg(id).arg(role).arg(sender).arg(text));
}

void ChatRoomWidget::onHistory(const QJsonArray& msgs, bool /*hasMore*/) {
    // 服务端历史返回按 id DESC，这里先从末尾插（较早的）以实现正序展示
    QJsonArray arr = msgs;
    for (int i=arr.size()-1;i>=0;--i) appendMessage(arr.at(i).toObject());
    if (!msgs.isEmpty()) {
        qint64 earliest = msgs.last().toObject().value("id").toVariant().toLongLong();
        m_earliestByPeer[m_currentPeer] = earliest;
    }
}

void ChatRoomWidget::onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool /*hasMore*/) {
    for (const auto &v : msgs) appendMessage(v.toObject());
    for (const auto &e : instant) {
        auto o = e.toObject();
        m_list->addItem(QString("[事件] %1 %2-%3")
                        .arg(o.value("event_type").toString())
                        .arg(o.value("doctor_user").toString())
                        .arg(o.value("patient_user").toString()));
    }
    m_cursor = nextCursor;
    schedulePoll();
}

void ChatRoomWidget::schedulePoll() {
    // 采用长轮询：上一次返回后立刻发起下一次
    QTimer::singleShot(0, this, [this]() { m_chat->pollEvents(m_cursor); });
}

void ChatRoomWidget::insertOlderAtTop(const QJsonArray &msgsDesc) {
    // msgsDesc 为 id DESC，需倒序插到顶部
    int insertPos = 0;
    for (int i=msgsDesc.size()-1;i>=0;--i) {
        auto o = msgsDesc.at(i).toObject();
        const QString sender = o.value("sender_username").toString(o.value("sender").toString());
        const QString text = o.value("text_content").toString();
        const int id = o.value("id").toInt();
        auto *item = new QListWidgetItem(QString("#%1 [%2]: %3").arg(id).arg(sender).arg(text));
        m_list->insertItem(insertPos++, item);
    }
}

void ChatRoomWidget::loadMore() {
    if (m_currentPeer.isEmpty()) return;
    const qint64 beforeId = m_earliestByPeer.value(m_currentPeer, 0);
    const qint64 useBefore = beforeId>0? beforeId : 0;
    m_chat->getHistory(m_doctor, m_currentPeer, useBefore, 20);
}
