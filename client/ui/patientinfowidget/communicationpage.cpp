#include "communicationpage.h"
#include "core/network/communicationclient.h"
#include "core/services/chatservice.h"
#include "core/services/doctorlistservice.h"
#include "ui/common/chatbubbledelegate.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

CommunicationPage::CommunicationPage(CommunicationClient* c, const QString& p, QWidget* parent)
    : BasePage(c, p, parent)
{
    auto* root = new QVBoxLayout(this);
    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel(QString("选择医生:")));
    m_doctorCombo = new QComboBox(this);
    top->addWidget(m_doctorCombo, 1);
    m_loadMoreBtn = new QPushButton("加载更多历史", this);
    top->addWidget(m_loadMoreBtn);
    root->addLayout(top);

    m_list = new QListWidget(this);
    m_list->setItemDelegate(new ChatBubbleDelegate(m_patientName, m_list));
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setSpacing(8);
    root->addWidget(m_list, 1);

    auto* bottom = new QHBoxLayout();
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("输入消息，回车发送");
    m_sendBtn = new QPushButton("发送", this);
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_sendBtn);
    root->addLayout(bottom);

    m_chat = new ChatService(m_client, m_patientName, this);
    m_doctorService = new DoctorListService(m_client, this);

    connect(m_sendBtn, &QPushButton::clicked, this, &CommunicationPage::sendClicked);
    connect(m_input, &QLineEdit::returnPressed, this, &CommunicationPage::sendClicked);
    connect(m_doctorCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &CommunicationPage::onPeerChanged);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &CommunicationPage::loadMore);
    // 不使用旧信号直接渲染，避免重复
    // connect(m_chat, &ChatService::historyReceived, this, &CommunicationPage::onHistory);
    connect(m_chat, &ChatService::eventsReceived, this, &CommunicationPage::onEvents);
    connect(m_chat, &ChatService::conversationHistoryLoaded, this, [this](const QString& doctor,const QString& patient,const QList<QJsonObject>& pageAsc,bool /*hasMore*/, qint64 earliest){
        if (patient!=m_patientName || doctor!=currentDoctor()) return;
        for (const auto &o : pageAsc) appendMessage(o);
        m_earliestByPeer[doctor] = earliest;
    });
    connect(m_chat, &ChatService::conversationUpserted, this, [this](const QString& doctor,const QString& patient,const QList<QJsonObject>& deltaAsc){
        if (patient!=m_patientName || doctor!=currentDoctor()) return;
        for (const auto &o : deltaAsc) appendMessage(o);
    });

    // 初始化医生下拉
    populateDoctors();
}

void CommunicationPage::populateDoctors() {
    connect(m_doctorService, &DoctorListService::fetched, this, [this](const QJsonArray& doctors){
        m_doctorCombo->clear();
        for (const auto &v : doctors) {
            const auto o = v.toObject();
            m_doctorCombo->addItem(QString("%1(%2)").arg(o.value("name").toString()).arg(o.value("department").toString()), o.value("username").toString());
        }
        // 不自动选中，等待用户选择
    });
    m_doctorService->fetchAllDoctors();
}

QString CommunicationPage::currentDoctor() const {
    return m_doctorCombo->currentIndex()>=0 ? m_doctorCombo->currentData().toString() : QString();
}

void CommunicationPage::sendClicked()
{
    const QString doctor = currentDoctor();
    if (doctor.isEmpty()) return;
    if (!m_input->text().isEmpty()) {
        m_chat->sendText(doctor, m_patientName, m_input->text());
        m_input->clear();
    }
}

void CommunicationPage::appendMessage(const QJsonObject& m)
{
    const QString doctor = m.value("doctor_username").toString();
    const QString patient = m.value("patient_username").toString();
    if (!(doctor == currentDoctor() && patient == m_patientName)) return;
    const QString sender = m.value("sender_username").toString(m.value("sender").toString());
    const QString text = m.value("text_content").toString();
    const int id = m.value("id").toInt();
    auto *item = new QListWidgetItem();
    item->setData(ChatItemRoles::RoleSender, sender);
    item->setData(ChatItemRoles::RoleText, text);
    item->setData(ChatItemRoles::RoleIsOutgoing, sender == m_patientName);
    item->setData(ChatItemRoles::RoleMessageId, id);
    item->setSizeHint(QSize(item->sizeHint().width(), 48));
    m_list->addItem(item);
    m_list->scrollToBottom();
}

void CommunicationPage::onHistory(const QJsonArray& msgs, bool /*hasMore*/){ Q_UNUSED(msgs); }

void CommunicationPage::onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool /*hasMore*/)
{
    Q_UNUSED(msgs); // 增量渲染由 conversationUpserted 处理
    for (const auto& e : instant) {
        auto o = e.toObject();
        // 可选：事件提示
        Q_UNUSED(o)
    }
    m_cursor = nextCursor; // 仅更新本地游标
}

void CommunicationPage::insertOlderAtTop(const QJsonArray& msgsDesc)
{
    int insertPos = 0;
    for (int i = msgsDesc.size() - 1; i >= 0; --i) {
        auto o = msgsDesc.at(i).toObject();
        const QString sender = o.value("sender_username").toString(o.value("sender").toString());
        const QString text = o.value("text_content").toString();
        const int id = o.value("id").toInt();
        auto* item = new QListWidgetItem();
        item->setData(ChatItemRoles::RoleSender, sender);
        item->setData(ChatItemRoles::RoleText, text);
        item->setData(ChatItemRoles::RoleIsOutgoing, sender == m_patientName);
        item->setData(ChatItemRoles::RoleMessageId, id);
        m_list->insertItem(insertPos++, item);
    }
}

void CommunicationPage::onPeerChanged(int /*idx*/)
{
    m_list->clear();
    if (currentDoctor().isEmpty()) { m_chat->stopPolling(); return; }
    // 先渲染缓存
    for (const auto &o : m_chat->messagesFor(currentDoctor(), m_patientName)) appendMessage(o);
    m_earliestByPeer[currentDoctor()] = m_chat->earliestIdFor(currentDoctor(), m_patientName);
    // 拉取最近 20
    m_chat->getHistory(currentDoctor(), m_patientName, 0, 20);
    m_chat->startPolling();
}

void CommunicationPage::loadMore()
{
    if (currentDoctor().isEmpty()) return;
    const qint64 beforeId = m_earliestByPeer.value(currentDoctor(), 0);
    const qint64 useBefore = beforeId > 0 ? beforeId : 0;
    m_chat->getHistory(currentDoctor(), m_patientName, useBefore, 20);
}
