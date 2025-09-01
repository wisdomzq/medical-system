#include "placeholderpages.h"
#include "core/network/communicationclient.h"
#include "core/services/chatservice.h"
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

// 为其他占位页面添加简单标签
template <typename T>
static void ensureLayout(T* w, const char* text)
{
    auto* lay = new QVBoxLayout(w);
    lay->addWidget(new QLabel(QString::fromUtf8(text)));
    lay->addStretch();
}

CommunicationPage::CommunicationPage(CommunicationClient* c, const QString& p, QWidget* parent)
    : BasePage(c, p, parent)
{
    auto* root = new QHBoxLayout(this);
    // 左侧：会话列表
    auto* left = new QVBoxLayout();
    left->addWidget(new QLabel(QString("患者聊天 - %1").arg(m_patientName)));
    m_convList = new QListWidget(this);
    left->addWidget(m_convList, 1);
    auto* addLine = new QHBoxLayout();
    m_peerEdit = new QLineEdit(this);
    m_peerEdit->setPlaceholderText("输入医生用户名，例如 dr.bob");
    m_addPeerBtn = new QPushButton("添加会话", this);
    addLine->addWidget(m_peerEdit, 1);
    addLine->addWidget(m_addPeerBtn);
    left->addLayout(addLine);

    // 右侧：消息与控制
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

    m_chat = new ChatService(m_client, m_patientName, this);
    connect(m_sendBtn, &QPushButton::clicked, this, &CommunicationPage::sendClicked);
    connect(m_input, &QLineEdit::returnPressed, this, &CommunicationPage::sendClicked);
    connect(m_addPeerBtn, &QPushButton::clicked, this, &CommunicationPage::addPeer);
    connect(m_convList, &QListWidget::currentRowChanged, this, &CommunicationPage::onPeerChanged);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &CommunicationPage::loadMore);
    connect(m_chat, &ChatService::historyReceived, this, &CommunicationPage::onHistory);
    connect(m_chat, &ChatService::eventsReceived, this, &CommunicationPage::onEvents);
    connect(m_chat, &ChatService::sendMessageResult, this, [this](bool ok, const QJsonObject&) { if(ok){} });
    connect(m_chat, &ChatService::recentContactsReceived, this, [this](const QJsonArray& contacts) {
        m_convList->clear();
        for (const auto& v : contacts)
            m_convList->addItem(v.toObject().value("username").toString());
        if (m_convList->count() > 0)
            m_convList->setCurrentRow(0);
    });
    // 自动初始化最近会话
    m_chat->recentContacts(20);
    schedulePoll();
}

void CommunicationPage::sendClicked()
{
    auto* item = m_convList->currentItem();
    if (!item)
        return;
    const QString peer = item->text();
    if (!m_input->text().isEmpty()) {
        m_chat->sendText(peer, m_patientName, m_input->text());
        m_input->clear();
    }
}

void CommunicationPage::appendMessage(const QJsonObject& m)
{
    // 仅显示当前选择的医生对应会话
    auto* item = m_convList->currentItem();
    if (!item)
        return;
    const QString currentDoctor = item->text();
    const QString doctor = m.value("doctor_username").toString();
    const QString patient = m.value("patient_username").toString();
    if (!(doctor == currentDoctor && patient == m_patientName))
        return;
    const QString sender = m.value("sender_username").toString(m.value("sender").toString());
    const QString text = m.value("text_content").toString();
    const int id = m.value("id").toInt();
    QString role = (sender == m_patientName) ? "患者" : "医生";
    m_list->addItem(QString("#%1 [%2][%3]: %4").arg(id).arg(role).arg(sender).arg(text));
}

void CommunicationPage::onHistory(const QJsonArray& msgs, bool /*hasMore*/)
{
    // 服务端历史为 id DESC，这里倒序插入实现正序展示
    for (int i = msgs.size() - 1; i >= 0; --i)
        appendMessage(msgs.at(i).toObject());
    if (!msgs.isEmpty()) {
        const qint64 earliest = msgs.last().toObject().value("id").toVariant().toLongLong();
        if (auto* item = m_convList->currentItem())
            m_earliestByPeer[item->text()] = earliest;
    }
}
void CommunicationPage::onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool /*hasMore*/)
{
    for (const auto& v : msgs)
        appendMessage(v.toObject());
    for (const auto& e : instant) {
        auto o = e.toObject();
        m_list->addItem(QString("[事件] %1 %2-%3").arg(o.value("event_type").toString()).arg(o.value("doctor_user").toString()).arg(o.value("patient_user").toString()));
    }
    m_cursor = nextCursor;
    schedulePoll();
}
void CommunicationPage::schedulePoll()
{
    QTimer::singleShot(0, this, [this]() { m_chat->pollEvents(m_cursor); });
}

void CommunicationPage::insertOlderAtTop(const QJsonArray& msgsDesc)
{
    int insertPos = 0;
    for (int i = msgsDesc.size() - 1; i >= 0; --i) {
        auto o = msgsDesc.at(i).toObject();
        const QString sender = o.value("sender_username").toString(o.value("sender").toString());
        const QString text = o.value("text_content").toString();
        const int id = o.value("id").toInt();
        auto* item = new QListWidgetItem(QString("#%1 [%2]: %3").arg(id).arg(sender).arg(text));
        m_list->insertItem(insertPos++, item);
    }
}

void CommunicationPage::addPeer()
{
    const QString peer = m_peerEdit->text().trimmed();
    if (peer.isEmpty())
        return;
    for (int i = 0; i < m_convList->count(); ++i) {
        if (m_convList->item(i)->text() == peer) {
            m_convList->setCurrentRow(i);
            return;
        }
    }
    m_convList->addItem(peer);
    m_convList->setCurrentRow(m_convList->count() - 1);
}
void CommunicationPage::onPeerChanged()
{
    m_list->clear();
    auto* item = m_convList->currentItem();
    if (!item)
        return;
    const QString peer = item->text();
    m_earliestByPeer[peer] = 0;
    m_chat->getHistory(peer, m_patientName, 0, 20);
}
void CommunicationPage::loadMore()
{
    auto* item = m_convList->currentItem();
    if (!item)
        return;
    const QString peer = item->text();
    const qint64 beforeId = m_earliestByPeer.value(peer, 0);
    const qint64 useBefore = beforeId > 0 ? beforeId : 0;
    m_chat->getHistory(peer, m_patientName, useBefore, 20);
}

HealthAssessmentPage::HealthAssessmentPage(CommunicationClient* c, const QString& p, QWidget* parent)
    : BasePage(c, p, parent)
{
    ensureLayout(this, "健康评估");
}
