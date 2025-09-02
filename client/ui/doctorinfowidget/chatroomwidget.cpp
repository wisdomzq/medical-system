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
#include <QFile>
#include <QIODevice>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
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

ChatRoomWidget::ChatRoomWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent), m_doctor(doctorName), m_client(client) {
    // 加载聊天室专用样式
    {
        QFile f(":/doctor_chatroom.qss");
        if (f.open(QIODevice::ReadOnly)) {
            this->setStyleSheet(QString::fromUtf8(f.readAll()));
        }
    }

    // 根布局：上方标题栏 + 下方内容区
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(0); // 移除间距让卡片紧贴

    // 顶部栏 - 包含标题和加载按钮
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("chatTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    topBar->setAutoFillBackground(true);
    // 颜色与尺寸完全交由 QSS 管理（与 profile/appointment 保持一致）
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(16, 12, 16, 12);
    topLay->setSpacing(8);
    auto* title = new QLabel(tr("医生聊天室"), topBar);
    title->setObjectName("chatTitle");
    title->setAttribute(Qt::WA_StyledBackground, true);
    title->setStyleSheet("background: transparent; border: none; margin: 0px; padding: 0px;");
    // 使按钮成为顶栏的子控件，便于应用 "QWidget#chatTopBar QPushButton" 的QSS规则
    m_loadMoreBtn = new QPushButton(tr("加载更多历史"), topBar);
    m_loadMoreBtn->setObjectName("loadMoreBtn");
    topLay->addWidget(title);
    topLay->addStretch();
    topLay->addWidget(m_loadMoreBtn);
    root->addWidget(topBar);

    // 内容区（左右）- 卡片布局
    auto* content = new QHBoxLayout();
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(12);
    root->addLayout(content, 1);

    // 左侧：会话列表卡片
    QWidget* leftCard = new QWidget(this);
    leftCard->setObjectName("leftCard");
    leftCard->setAttribute(Qt::WA_StyledBackground, true);
    leftCard->setAutoFillBackground(true);
    auto* left = new QVBoxLayout(leftCard);
    left->setContentsMargins(0, 0, 0, 0);
    left->setSpacing(0);
    
    m_convList = new QListWidget(this);
    m_convList->setObjectName("convList");
    m_convList->setViewMode(QListView::ListMode);
    m_convList->setFlow(QListView::TopToBottom);
    m_convList->setIconSize(QSize(40, 40));
    m_convList->setSpacing(0);
    m_convList->setUniformItemSizes(false);
    m_convList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_convList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_convList->setMinimumHeight(400);
    m_convList->setMaximumHeight(600);
    left->addWidget(m_convList, 1);

    // 医生端不允许主动添加患者会话，移除输入与按钮

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
    m_list->setItemDelegate(new ChatBubbleDelegate(m_doctor, m_list));
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setSpacing(8);
    m_list->setMinimumHeight(400);
    m_list->setMaximumHeight(600);
    right->addWidget(m_list, 1);

    // 装入内容区
    content->addWidget(leftCard, 1);
    content->addWidget(rightCard, 2);

    // 底部输入区域 - 独立于卡片，与卡片边距对齐
    auto bottom = new QHBoxLayout();
    bottom->setContentsMargins(0, 8, 0, 0); // 上方间距8px，左右与卡片对齐
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(tr("输入消息，回车发送"));
    m_sendBtn = new QPushButton(tr("发送"), this);
    m_sendImageBtn = new QPushButton(tr("发送图片"), this);
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_sendBtn);
    bottom->addWidget(m_sendImageBtn);
    root->addLayout(bottom); // 添加到根布局而不是右侧卡片

    m_chat = new ChatService(m_client, m_doctor, this);
    connect(m_sendBtn, &QPushButton::clicked, this, &ChatRoomWidget::sendClicked);
    connect(m_sendImageBtn, &QPushButton::clicked, this, &ChatRoomWidget::sendImageClicked);
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
        // 清除当前会话的未读徽标
        for (int i=0;i<m_convList->count();++i){
            auto *it = m_convList->item(i); if (!it) continue; if (it->text()!=m_currentPeer) continue;
            if (auto *w = m_convList->itemWidget(it)){
                if (auto *bd = w->findChild<QLabel*>("convBadge")) {
                    w->setProperty("unread", 0); bd->hide();
                }
            }
        }
        m_earliestByPeer[m_currentPeer] = earliest;
    });
    connect(m_chat, &ChatService::conversationUpserted, this, [this](const QString& doctor,const QString& patient,const QList<QJsonObject>& deltaAsc){
        if (doctor!=m_doctor || patient!=m_currentPeer) return;
        for (const auto &o : deltaAsc) {
            appendMessage(o);
            const QString peer = o.value("patient_username").toString();
            const QString sender = o.value("sender_username").toString(o.value("sender").toString());
            // 更新对应会话行的未读
            for (int i=0;i<m_convList->count();++i){
                auto *it = m_convList->item(i); if (!it) continue; if (it->text()!=peer) continue;
                if (auto *w = m_convList->itemWidget(it)){
                    int unread = w->property("unread").toInt();
                    if (peer!=m_currentPeer && sender!=m_doctor) unread += 1; else unread = 0; // 当前会话不累积
                    w->setProperty("unread", unread);
                    if (auto *bd = w->findChild<QLabel*>("convBadge")) {
                        if (unread>0) { bd->setText(QString::number(unread)); bd->show(); }
                        else { bd->hide(); }
                    }
                }
            }
        }
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
        const QIcon avatarIcon(":/icons/用户.svg");
        for (const auto &v : contacts) {
            const QString name = v.toObject().value("username").toString();
            // 行项目
            auto *it = new QListWidgetItem();
            it->setText(name);                   // 逻辑上仍使用文本
            it->setData(Qt::UserRole, name);     // 备用：存储用户名
            it->setSizeHint(QSize(200, 60));     // 行高
            m_convList->addItem(it);

            // 行内容部件：圆形头像 + 名称
            QWidget *row = new QWidget(m_convList);
            row->setObjectName("convRow");
            row->setAttribute(Qt::WA_StyledBackground, true);
            row->setAutoFillBackground(true);
            // 直接设置样式表强制背景色，使用更不透明的颜色
            row->setStyleSheet("QWidget#convRow { background-color: rgb(240,248,255) !important; border-bottom: 1px solid rgba(0,0,0,15); }");
            auto *hl = new QHBoxLayout(row);
            hl->setContentsMargins(12, 10, 12, 10);
            hl->setSpacing(10);
            auto *av = new QLabel(row);
            av->setFixedSize(40, 40);
            av->setObjectName("convAvatar");
            av->setPixmap(makeRoundAvatar(avatarIcon, 40));
            av->setStyleSheet("background: transparent;"); // 头像背景透明
            // 文本区域：仅名称
            auto *textBox = new QVBoxLayout();
            textBox->setContentsMargins(0,0,0,0);
            textBox->setSpacing(0);
            auto *nm = new QLabel(name, row);
            nm->setObjectName("convName");
            nm->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            nm->setStyleSheet("background: transparent; color: black;"); // 名称标签背景透明，文字黑色
            textBox->addWidget(nm);

            // 未读徽标
            auto *badge = new QLabel("", row);
            badge->setObjectName("convBadge");
            badge->setFixedSize(22,22);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet("background: transparent;"); // 徽标背景透明
            badge->hide();

            hl->addWidget(av);
            hl->addLayout(textBox, 1);
            hl->addWidget(badge);
            row->setLayout(hl);
            row->setProperty("active", false); // 默认未选中
            row->setProperty("unread", 0);
            row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            m_convList->setItemWidget(it, row);
        }
        // 初始不自动选中与加载，等待用户选择
    });
    // 仅加载最近联系人，不自动轮询
    m_chat->recentContacts(20);

    // 监听文件下载完成，刷新对应图片项
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
    // 完成一个，继续队列
    m_downloading = false;
    tryStartNextDownload();
    });
}

void ChatRoomWidget::sendClicked() {
    if (m_currentPeer.isEmpty()) return;
    if (!m_input->text().isEmpty()) {
        m_chat->sendText(m_doctor, m_currentPeer, m_input->text());
        m_input->clear();
    }
}

void ChatRoomWidget::sendImageClicked() {
    if (m_currentPeer.isEmpty()) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("选择图片"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (path.isEmpty()) return;
    qInfo() << "[ ChatRoomWidget ] 选择图片" << path;
    m_chat->sendImage(m_doctor, m_currentPeer, path);
}

void ChatRoomWidget::addPeer() { /* 医生端禁用 */ }

void ChatRoomWidget::onPeerChanged() {
    auto *item = m_convList->currentItem();
    // 更新行样式：先清除所有 active，再给当前设置 active
    for (int i=0;i<m_convList->count();++i){
        if (auto *it = m_convList->item(i)){
            if (auto *w = m_convList->itemWidget(it)){
                const bool isActive = (it==item);
                w->setProperty("active", isActive);
                if (isActive) { 
                    w->setProperty("unread", 0); 
                    if (auto *bd=w->findChild<QLabel*>("convBadge")) bd->hide();
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
    const QString msgType = m.value("message_type").toString("text");
    const QString serverImage = m.value("file_metadata").toObject().value("path").toString();
    const int id = m.value("id").toInt();
    auto *item = new QListWidgetItem();
    item->setData(ChatItemRoles::RoleSender, sender);
    item->setData(ChatItemRoles::RoleText, text);
    item->setData(ChatItemRoles::RoleIsOutgoing, sender == m_doctor);
    item->setData(ChatItemRoles::RoleMessageId, id);
    item->setData(ChatItemRoles::RoleMessageType, msgType);
    if (msgType == "image") {
        item->setSizeHint(QSize(item->sizeHint().width(), 160));
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/chat_images";
        QDir().mkpath(cacheDir);
        const QString localPath = QDir(cacheDir).filePath(serverImage);
        item->setData(ChatItemRoles::RoleImageServerName, serverImage);
        if (QFile::exists(localPath)) {
            qInfo() << "[ ChatRoomWidget ] 使用已缓存图片" << localPath;
            item->setData(ChatItemRoles::RoleImageLocalPath, localPath);
        } else if (!serverImage.isEmpty()) {
            item->setData(ChatItemRoles::RoleImageLocalPath, QString());
            // 入队并尝试串行下载
            qInfo() << "[ ChatRoomWidget ] 触发下载图片 server=" << serverImage << ", local=" << localPath;
            m_downloadQueue.enqueue(qMakePair(serverImage, localPath));
            tryStartNextDownload();
        }
    } else {
        item->setSizeHint(QSize(item->sizeHint().width(), 48));
    }
    m_list->addItem(item);
    m_list->scrollToBottom();
}

void ChatRoomWidget::tryStartNextDownload() {
    if (m_downloading || m_downloadQueue.isEmpty()) return;
    const auto pair = m_downloadQueue.dequeue();
    qInfo() << "[ ChatRoomWidget ] 开始下载 server=" << pair.first << ", local=" << pair.second;
    m_downloading = true;
    m_client->downloadFile(pair.first, pair.second);
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
