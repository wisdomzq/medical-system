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
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

// 本地工具：生成圆形头像
namespace {
static QPixmap makeRoundAvatar(const QIcon &icon, int size) {
    const QPixmap src = icon.pixmap(size, size);
    QPixmap dst(size, size);
    dst.fill(Qt::transparent);
    QPainter p(&dst);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path; path.addEllipse(0, 0, size, size);
    p.setClipPath(path);
    p.drawPixmap(0, 0, src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    p.end();
    return dst;
}
}

CommunicationPage::CommunicationPage(CommunicationClient* c, const QString& p, QWidget* parent)
    : BasePage(c, p, parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(0);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("communicationTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("医患沟通", topBar);
    title->setObjectName("communicationTitle");
    title->setAttribute(Qt::WA_StyledBackground, true);
    title->setStyleSheet("background: transparent; border: none; margin: 0px; padding: 0px;");
    
    m_loadMoreBtn = new QPushButton("加载更多历史", topBar);
    m_loadMoreBtn->setObjectName("primaryBtn");
    topBarLayout->addWidget(title);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_loadMoreBtn);
    root->addWidget(topBar);
    
    // 内容区（左右卡片布局）
    auto* content = new QHBoxLayout();
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(12);
    root->addLayout(content, 1);

    // 左侧：医生列表卡片
    QWidget* leftCard = new QWidget(this);
    leftCard->setObjectName("leftCard");
    leftCard->setAttribute(Qt::WA_StyledBackground, true);
    leftCard->setAutoFillBackground(true);
    auto* left = new QVBoxLayout(leftCard);
    left->setContentsMargins(0, 0, 0, 0);
    left->setSpacing(0);
    
    m_doctorList = new QListWidget(this);
    m_doctorList->setObjectName("convList");
    m_doctorList->setViewMode(QListView::ListMode);
    m_doctorList->setFlow(QListView::TopToBottom);
    m_doctorList->setIconSize(QSize(40, 40));
    m_doctorList->setSpacing(0);
    m_doctorList->setUniformItemSizes(false);
    m_doctorList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_doctorList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_doctorList->setMinimumHeight(400);
    m_doctorList->setMaximumHeight(600);
    left->addWidget(m_doctorList, 1);

    // 右侧：消息区卡片
    QWidget* rightCard = new QWidget(this);
    rightCard->setObjectName("rightCard");
    rightCard->setAttribute(Qt::WA_StyledBackground, true);
    rightCard->setAutoFillBackground(true);
    auto* right = new QVBoxLayout(rightCard);
    right->setContentsMargins(0, 0, 0, 0);
    right->setSpacing(0);

    // 聊天内容区域
    m_list = new QListWidget(this);
    m_list->setItemDelegate(new ChatBubbleDelegate(m_patientName, m_list));
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setSpacing(8);
    m_list->setMinimumHeight(400);
    m_list->setMaximumHeight(600);
    right->addWidget(m_list, 1);

    // 装入内容区
    content->addWidget(leftCard, 1);
    content->addWidget(rightCard, 2);

    // 底部输入区域 - 独立于卡片，与卡片边距对齐
    auto* bottom = new QHBoxLayout();
    bottom->setContentsMargins(0, 8, 0, 0);
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("输入消息，回车发送");
    m_sendBtn = new QPushButton("发送", this);
    m_sendImageBtn = new QPushButton("发送图片", this);
    m_sendBtn->setObjectName("primaryBtn");
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_sendBtn);
    bottom->addWidget(m_sendImageBtn);
    root->addLayout(bottom);

    m_chat = new ChatService(m_client, m_patientName, this);
    m_doctorService = new DoctorListService(m_client, this);

    connect(m_sendBtn, &QPushButton::clicked, this, &CommunicationPage::sendClicked);
    connect(m_input, &QLineEdit::returnPressed, this, &CommunicationPage::sendClicked);
    connect(m_sendImageBtn, &QPushButton::clicked, this, &CommunicationPage::sendImageClicked);
    connect(m_doctorList, &QListWidget::currentRowChanged, this, &CommunicationPage::onPeerChanged);
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

    // 监听下载完成（刷新图片项）
    connect(m_client, &CommunicationClient::fileDownloaded, this, [this](const QString& serverName, const QString& localPath){
        for (int i=0;i<m_list->count();++i) {
            auto *it = m_list->item(i);
            if (!it) continue;
            if (it->data(ChatItemRoles::RoleMessageType).toString()=="image" && it->data(ChatItemRoles::RoleImageServerName).toString()==serverName) {
                it->setData(ChatItemRoles::RoleImageLocalPath, localPath);
                const auto r = m_list->visualItemRect(it);
                m_list->viewport()->update(r);
            }
        }
        m_downloading = false;
        tryStartNextDownload();
    });
}

void CommunicationPage::populateDoctors() {
    connect(m_doctorService, &DoctorListService::fetched, this, [this](const QJsonArray& doctors){
        m_doctorList->clear();
        const QIcon avatarIcon(":/icons/医生.svg");
        for (const auto &v : doctors) {
            const auto o = v.toObject();
            const QString username = o.value("username").toString();
            const QString name = o.value("name").toString();
            const QString department = o.value("department").toString();
            const QString displayName = QString("%1(%2)").arg(name).arg(department);
            
            // 行项目
            auto *it = new QListWidgetItem();
            it->setText(username);                   // 逻辑上使用用户名
            it->setData(Qt::UserRole, username);     // 备用：存储用户名
            it->setSizeHint(QSize(200, 60));         // 行高
            m_doctorList->addItem(it);

            // 行内容部件：圆形头像 + 名称
            QWidget *row = new QWidget(m_doctorList);
            row->setObjectName("convRow");
            row->setAttribute(Qt::WA_StyledBackground, true);
            row->setAutoFillBackground(true);
            row->setStyleSheet("QWidget#convRow { background-color: rgb(240,248,255) !important; border-bottom: 1px solid rgba(0,0,0,15); }");
            auto *hl = new QHBoxLayout(row);
            hl->setContentsMargins(12, 10, 12, 10);
            hl->setSpacing(10);
            auto *av = new QLabel(row);
            av->setFixedSize(40, 40);
            av->setObjectName("convAvatar");
            av->setPixmap(makeRoundAvatar(avatarIcon, 40));
            av->setStyleSheet("background: transparent;");
            
            // 文本区域：医生名称和科室
            auto *textBox = new QVBoxLayout();
            textBox->setContentsMargins(0,0,0,0);
            textBox->setSpacing(0);
            auto *nm = new QLabel(displayName, row);
            nm->setObjectName("convName");
            nm->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            nm->setStyleSheet("background: transparent; color: black;");
            textBox->addWidget(nm);

            hl->addWidget(av);
            hl->addLayout(textBox, 1);
            row->setLayout(hl);
            row->setProperty("active", false); // 默认未选中
            row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            m_doctorList->setItemWidget(it, row);
        }
    });
    m_doctorService->fetchAllDoctors();
}

QString CommunicationPage::currentDoctor() const {
    auto *item = m_doctorList->currentItem();
    return item ? item->text() : QString();
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

void CommunicationPage::sendImageClicked()
{
    const QString doctor = currentDoctor();
    if (doctor.isEmpty()) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("选择图片"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (path.isEmpty()) return;
    m_chat->sendImage(doctor, m_patientName, path);
}

void CommunicationPage::appendMessage(const QJsonObject& m)
{
    const QString doctor = m.value("doctor_username").toString();
    const QString patient = m.value("patient_username").toString();
    if (!(doctor == currentDoctor() && patient == m_patientName)) return;
    const QString sender = m.value("sender_username").toString(m.value("sender").toString());
    const QString text = m.value("text_content").toString();
    const QString msgType = m.value("message_type").toString("text");
    const QString serverImage = m.value("file_metadata").toObject().value("path").toString();
    const int id = m.value("id").toInt();
    auto *item = new QListWidgetItem();
    item->setData(ChatItemRoles::RoleSender, sender);
    item->setData(ChatItemRoles::RoleText, text);
    item->setData(ChatItemRoles::RoleIsOutgoing, sender == m_patientName);
    item->setData(ChatItemRoles::RoleMessageId, id);
    item->setData(ChatItemRoles::RoleMessageType, msgType);
    if (msgType == "image") {
        item->setSizeHint(QSize(item->sizeHint().width(), 160));
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/chat_images";
        QDir().mkpath(cacheDir);
        const QString localPath = QDir(cacheDir).filePath(serverImage);
        item->setData(ChatItemRoles::RoleImageServerName, serverImage);
        if (QFile::exists(localPath)) {
            qInfo() << "[ CommunicationPage ] 使用已缓存图片" << localPath;
            item->setData(ChatItemRoles::RoleImageLocalPath, localPath);
        } else if (!serverImage.isEmpty()) {
            item->setData(ChatItemRoles::RoleImageLocalPath, QString());
            qInfo() << "[ CommunicationPage ] 触发下载图片 server=" << serverImage << ", local=" << localPath;
            m_downloadQueue.enqueue(qMakePair(serverImage, localPath));
            tryStartNextDownload();
        }
    } else {
        item->setSizeHint(QSize(item->sizeHint().width(), 48));
    }
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

void CommunicationPage::onPeerChanged()
{
    auto *item = m_doctorList->currentItem();
    // 更新行样式：先清除所有 active，再给当前设置 active
    for (int i=0;i<m_doctorList->count();++i){
        if (auto *it = m_doctorList->item(i)){
            if (auto *w = m_doctorList->itemWidget(it)){
                const bool isActive = (it==item);
                w->setProperty("active", isActive);
                if (isActive) { 
                    // 设置选中状态的背景色 - 深蓝色
                    w->setStyleSheet("QWidget#convRow { background-color: rgba(207, 189, 241, 1) !important; border-bottom: 1px solid rgba(0,0,0,15); }");
                } else {
                    // 设置默认状态的背景色 - 浅蓝色
                    w->setStyleSheet("QWidget#convRow { background-color: rgba(240,248,255, 1) !important; border-bottom: 1px solid rgba(0,0,0,15); }");
                }
                w->style()->unpolish(w); w->style()->polish(w); w->update();
            }
        }
    }
    
    m_list->clear();
    if (!item) { m_currentPeer.clear(); m_chat->stopPolling(); return; }
    m_currentPeer = item->text();
    
    // 先渲染缓存
    for (const auto &o : m_chat->messagesFor(m_currentPeer, m_patientName)) appendMessage(o);
    m_earliestByPeer[m_currentPeer] = m_chat->earliestIdFor(m_currentPeer, m_patientName);
    // 拉取最近 20
    m_chat->getHistory(m_currentPeer, m_patientName, 0, 20);
    m_chat->startPolling();
}

void CommunicationPage::loadMore()
{
    if (currentDoctor().isEmpty()) return;
    const qint64 beforeId = m_earliestByPeer.value(currentDoctor(), 0);
    const qint64 useBefore = beforeId > 0 ? beforeId : 0;
    m_chat->getHistory(currentDoctor(), m_patientName, useBefore, 20);
}

void CommunicationPage::tryStartNextDownload()
{
    if (m_downloading || m_downloadQueue.isEmpty()) return;
    const auto pair = m_downloadQueue.dequeue();
    m_downloading = true;
    m_client->downloadFile(pair.first, pair.second);
}
