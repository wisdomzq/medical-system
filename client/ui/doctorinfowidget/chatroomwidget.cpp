#include "chatroomwidget.h"
#include "core/services/chatservice.h"
#include "core/network/communicationclient.h"
#include "ui/common/chatbubbledelegate.h"
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
    // 医生端不允许主动添加患者会话，移除输入与按钮

    // 右侧：消息区
    auto* right = new QVBoxLayout();
    auto ctrl = new QHBoxLayout();
    m_loadMoreBtn = new QPushButton("加载更多历史", this);
    ctrl->addWidget(m_loadMoreBtn);
    ctrl->addStretch();
    right->addLayout(ctrl);

    m_list = new QListWidget(this);
    m_list->setItemDelegate(new ChatBubbleDelegate(m_doctor, m_list));
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setSpacing(8);
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
    // 医生端不允许 addPeer
    connect(m_convList, &QListWidget::currentRowChanged, this, &ChatRoomWidget::onPeerChanged);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &ChatRoomWidget::loadMore);
    // 不直接使用旧的 historyReceived 渲染，避免重复
    // connect(m_chat, &ChatService::historyReceived, this, &ChatRoomWidget::onHistory);
    connect(m_chat, &ChatService::eventsReceived, this, &ChatRoomWidget::onEvents);
    // 接入消息管理器
    connect(m_chat, &ChatService::conversationHistoryLoaded, this, [this](const QString& doctor,const QString& patient,const QList<QJsonObject>& pageAsc,bool /*hasMore*/, qint64 earliest){
        if (doctor!=m_doctor || patient!=m_currentPeer) return;
        for (const auto &o : pageAsc) appendMessage(o);
        m_earliestByPeer[m_currentPeer] = earliest;
    });
    connect(m_chat, &ChatService::conversationUpserted, this, [this](const QString& doctor,const QString& patient,const QList<QJsonObject>& deltaAsc){
        if (doctor!=m_doctor || patient!=m_currentPeer) return;
        for (const auto &o : deltaAsc) appendMessage(o);
    });
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
        // 初始不自动选中与加载，等待用户选择
    });
    // 仅加载最近联系人，不自动轮询
    m_chat->recentContacts(20);
}

void ChatRoomWidget::sendClicked() {
    if (m_currentPeer.isEmpty()) return;
    if (!m_input->text().isEmpty()) {
        m_chat->sendText(m_doctor, m_currentPeer, m_input->text());
        m_input->clear();
    }
}

void ChatRoomWidget::addPeer() { /* 医生端禁用 */ }

void ChatRoomWidget::onPeerChanged() {
    auto *item = m_convList->currentItem();
    m_list->clear();
    if (!item) { m_currentPeer.clear(); m_chat->stopPolling(); return; }
    m_currentPeer = item->text();
    // 先从本地消息管理器渲染（如有缓存）
    const auto cached = m_chat->messagesFor(m_doctor, m_currentPeer);
    for (const auto &o : cached) appendMessage(o);
    m_earliestByPeer[m_currentPeer] = m_chat->earliestIdFor(m_doctor, m_currentPeer);
    // 再拉取最近 20 条（beforeId=0）
    m_chat->getHistory(m_doctor, m_currentPeer, 0, 20);
    // 开启轮询（由 Service 串联）
    m_chat->startPolling();
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
    auto *item = new QListWidgetItem();
    item->setData(ChatItemRoles::RoleSender, sender);
    item->setData(ChatItemRoles::RoleText, text);
    item->setData(ChatItemRoles::RoleIsOutgoing, sender == m_doctor);
    item->setData(ChatItemRoles::RoleMessageId, id);
    item->setSizeHint(QSize(item->sizeHint().width(), 48));
    m_list->addItem(item);
    m_list->scrollToBottom();
}

void ChatRoomWidget::onHistory(const QJsonArray& msgs, bool /*hasMore*/) {
    Q_UNUSED(msgs);
}

void ChatRoomWidget::onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool /*hasMore*/) {
    Q_UNUSED(msgs); // 增量渲染由 conversationUpserted 处理
    for (const auto &e : instant) {
        auto o = e.toObject();
        m_list->addItem(QString("[事件] %1 %2-%3")
                        .arg(o.value("event_type").toString())
                        .arg(o.value("doctor_user").toString())
                        .arg(o.value("patient_user").toString()));
    }
    m_cursor = nextCursor; // 保留本地记录以便调试/显示
}

void ChatRoomWidget::insertOlderAtTop(const QJsonArray &msgsDesc) {
    // msgsDesc 为 id DESC，需倒序插到顶部
    int insertPos = 0;
    for (int i=msgsDesc.size()-1;i>=0;--i) {
        auto o = msgsDesc.at(i).toObject();
        const QString sender = o.value("sender_username").toString(o.value("sender").toString());
        const QString text = o.value("text_content").toString();
        const int id = o.value("id").toInt();
    auto *item = new QListWidgetItem();
    item->setData(ChatItemRoles::RoleSender, sender);
    item->setData(ChatItemRoles::RoleText, text);
    item->setData(ChatItemRoles::RoleIsOutgoing, sender == m_doctor);
    item->setData(ChatItemRoles::RoleMessageId, id);
        m_list->insertItem(insertPos++, item);
    }
}

void ChatRoomWidget::loadMore() {
    if (m_currentPeer.isEmpty()) return;
    const qint64 beforeId = m_earliestByPeer.value(m_currentPeer, 0);
    const qint64 useBefore = beforeId>0? beforeId : 0;
    m_chat->getHistory(m_doctor, m_currentPeer, useBefore, 20);
}
