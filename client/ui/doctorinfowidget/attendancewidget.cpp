#include "attendancewidget.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/attendanceservice.h"
#include <QFile>
#include <QIODevice>
#include <QDate>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

AttendanceWidget::AttendanceWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent)
    , doctorName_(doctorName)
    , client_(client)
{
    // 样式：加载专用 QSS
    {
        QFile f(":/doctor_attendance.qss");
        if (f.open(QIODevice::ReadOnly)) {
            this->setStyleSheet(QString::fromUtf8(f.readAll()));
        }
    }

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* nav = new QHBoxLayout();
    btnCheckIn_ = new QPushButton(tr("日常打卡"), this);
    btnLeave_ = new QPushButton(tr("请假"), this);
    btnCancel_ = new QPushButton(tr("销假"), this);
    // 分段按钮：checkable + autoExclusive 实现互斥高亮
    for (QPushButton* b : {btnCheckIn_, btnLeave_, btnCancel_}) {
        b->setCheckable(true);
        b->setAutoExclusive(true);
        b->setProperty("class", "SegBtn");
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    // 顶部操作条容器 + 胶囊容器
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("attTopBar");
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->setSpacing(0);

    QWidget* segGroup = new QWidget(topBar);
    segGroup->setObjectName("segGroup");
    auto* segLay = new QHBoxLayout(segGroup);
    segLay->setContentsMargins(6, 6, 6, 6); // 内边距形成圆角留白
    segLay->setSpacing(6);                  // 按钮之间留细缝
    // 三按钮等分铺满
    segLay->addWidget(btnCheckIn_, /*stretch*/1);
    segLay->addWidget(btnLeave_, /*stretch*/1);
    segLay->addWidget(btnCancel_, /*stretch*/1);
    segGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 胶囊容器填满右侧
    topLay->addWidget(segGroup, 1);
    root->addWidget(topBar);

    stack_ = new QStackedWidget(this);
    auto* pageCheckIn = new QWidget(this);
    {
        pageCheckIn->setProperty("class", "AttendanceRoot");
        auto* l = new QVBoxLayout(pageCheckIn);
        l->setSpacing(12);

        // 卡片：打卡操作
        auto* card = new QWidget(pageCheckIn);
        card->setProperty("class", "Card");
        auto* cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(16, 12, 16, 16);
        auto* title = new QLabel(QString("%1 - 日常打卡").arg(doctorName_), card);
        title->setProperty("class", "CardTitle");
        cardLay->addWidget(title);

        auto* form = new QHBoxLayout();
        form->setSpacing(12);
        auto* lbDate = new QLabel(tr("日期:"), card);
        auto* deDate = new QDateEdit(QDate::currentDate(), card);
        deDate->setDisplayFormat("yyyy-MM-dd");
        deDate->setCalendarPopup(true);
        auto* lbTime = new QLabel(tr("时间:"), card);
        auto* teTime = new QTimeEdit(QTime::currentTime(), card);
        teTime->setDisplayFormat("HH:mm:ss");
        btnDoCheckIn_ = new QPushButton(tr("打卡"), card);
        btnHistory_ = new QPushButton(tr("查看历史打卡"), card);
        btnHistory_->setProperty("class", "SecondaryBtn");

        form->addWidget(lbDate);
        form->addWidget(deDate);
        form->addWidget(lbTime);
        form->addWidget(teTime);
        form->addStretch();
        form->addWidget(btnDoCheckIn_);
        form->addWidget(btnHistory_);
        cardLay->addLayout(form);
        l->addWidget(card);

        // 卡片：历史记录
        auto* card2 = new QWidget(pageCheckIn);
        card2->setProperty("class", "Card");
        auto* card2Lay = new QVBoxLayout(card2);
        card2Lay->setContentsMargins(16, 12, 16, 16);
        auto* title2 = new QLabel(tr("历史打卡记录"), card2);
        title2->setProperty("class", "CardTitle");
        card2Lay->addWidget(title2);

        tblAttendance_ = new QTableWidget(0, 3, card2);
        tblAttendance_->setHorizontalHeaderLabels({ tr("日期"), tr("时间"), tr("创建时间") });
        tblAttendance_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        card2Lay->addWidget(tblAttendance_);
        l->addWidget(card2, 1);

        connect(btnDoCheckIn_, &QPushButton::clicked, this, [this, deDate, teTime] {
            if (service_)
                service_->checkIn(doctorName_, deDate->date().toString("yyyy-MM-dd"), teTime->time().toString("HH:mm:ss"));
        });
        connect(btnHistory_, &QPushButton::clicked, this, [this] { historyUserTriggered_ = true; refreshAttendanceHistory(); });
    }
    auto* pageLeave = new QWidget(this);
    {
        auto* l = new QVBoxLayout(pageLeave);
        l->setSpacing(12);
        pageLeave->setProperty("class", "AttendanceRoot");
        auto* card = new QWidget(pageLeave);
        card->setProperty("class", "Card");
        auto* cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(16, 12, 16, 16);
        auto* title = new QLabel(QString("%1 - 请假").arg(doctorName_), card);
        title->setProperty("class", "CardTitle");
        cardLay->addWidget(title);
        auto* row1 = new QHBoxLayout();
        row1->setSpacing(12);
        auto* lb = new QLabel(tr("请假日期:"), card);
        auto* de = new QDateEdit(QDate::currentDate(), card);
        de->setDisplayFormat("yyyy-MM-dd");
        de->setCalendarPopup(true);
        leLeaveDate_ = new QLineEdit(QDate::currentDate().toString("yyyy-MM-dd"), card); // 保留数据成员但不直接显示
        leLeaveDate_->setVisible(false);
        row1->addWidget(lb);
        row1->addWidget(de);
        cardLay->addLayout(row1);
        teReason_ = new QTextEdit(card);
        teReason_->setPlaceholderText(tr("请假原因..."));
        cardLay->addWidget(teReason_);
        btnSubmitLeave_ = new QPushButton(tr("保存请假"), card);
        cardLay->addWidget(btnSubmitLeave_);
        l->addWidget(card);

        connect(btnSubmitLeave_, &QPushButton::clicked, this, [this, de](){
            if (leLeaveDate_) leLeaveDate_->setText(de->date().toString("yyyy-MM-dd"));
            submitLeave();
        });
    }
    auto* pageCancel = new QWidget(this);
    {
    auto* l = new QVBoxLayout(pageCancel);
    l->setSpacing(12);
    pageCancel->setProperty("class", "AttendanceRoot");

    auto* card = new QWidget(pageCancel);
    card->setProperty("class", "Card");
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(16, 12, 16, 16);
    auto* title = new QLabel(QString("%1 - 销假").arg(doctorName_), card);
    title->setProperty("class", "CardTitle");
    cardLay->addWidget(title);
    auto* bar = new QHBoxLayout();
    btnRefreshLeaves_ = new QPushButton(tr("刷新请假记录"), card);
    btnRefreshLeaves_->setProperty("class", "SecondaryBtn");
    btnCancelLeave_ = new QPushButton(tr("销选中假"), card);
    bar->addWidget(btnRefreshLeaves_);
    bar->addWidget(btnCancelLeave_);
    bar->addStretch();
    cardLay->addLayout(bar);
    tblLeaves_ = new QTableWidget(0, 4, card);
    tblLeaves_->setHorizontalHeaderLabels({ tr("ID"), tr("日期"), tr("原因"), tr("状态") });
    tblLeaves_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    cardLay->addWidget(tblLeaves_);
    l->addWidget(card);

        connect(btnRefreshLeaves_, &QPushButton::clicked, this, &AttendanceWidget::refreshActiveLeaves);
        connect(btnCancelLeave_, &QPushButton::clicked, this, &AttendanceWidget::cancelSelectedLeave);
    }
    stack_->addWidget(pageCheckIn);
    stack_->addWidget(pageLeave);
    stack_->addWidget(pageCancel);
    root->addWidget(stack_);

    connect(btnCheckIn_, &QPushButton::clicked, this, &AttendanceWidget::showCheckIn);
    connect(btnLeave_, &QPushButton::clicked, this, &AttendanceWidget::showLeave);
    connect(btnCancel_, &QPushButton::clicked, this, &AttendanceWidget::showCancelLeave);

    showCheckIn();

    service_ = new AttendanceService(client_, this);
    connect(client_, &CommunicationClient::connected, this, &AttendanceWidget::onConnected);
    connect(service_, &AttendanceService::checkInResult, this, [this](bool ok, const QString& msg) {
        if (ok) {
            QMessageBox::information(this, tr("提示"), tr("打卡成功"));
            refreshAttendanceHistory();
        } else {
            QMessageBox::warning(this, tr("失败"), msg.isEmpty() ? tr("打卡失败") : msg);
        }
    });
    connect(service_, &AttendanceService::leaveSubmitted, this, [this](bool ok, const QString& msg, const QJsonObject& d) {
        showCancelLeave();
        if (ok) {
            int row = tblLeaves_->rowCount();
            tblLeaves_->insertRow(row);
            tblLeaves_->setItem(row, 0, new QTableWidgetItem("-"));
            tblLeaves_->setItem(row, 1, new QTableWidgetItem(d.value("leave_date").toString()));
            tblLeaves_->setItem(row, 2, new QTableWidgetItem(d.value("reason").toString()));
            tblLeaves_->setItem(row, 3, new QTableWidgetItem("active"));
        }
        Q_UNUSED(msg);
        refreshActiveLeaves();
    });
    connect(service_, &AttendanceService::activeLeavesReceived, this, [this](const QJsonArray& arr) {
        tblLeaves_->setRowCount(0);
        tblLeaves_->setRowCount(arr.size());
        for (int i = 0; i < arr.size(); ++i) {
            const auto o = arr[i].toObject();
            tblLeaves_->setItem(i, 0, new QTableWidgetItem(QString::number(o.value("id").toInt())));
            tblLeaves_->setItem(i, 1, new QTableWidgetItem(o.value("leave_date").toString()));
            tblLeaves_->setItem(i, 2, new QTableWidgetItem(o.value("reason").toString()));
            tblLeaves_->setItem(i, 3, new QTableWidgetItem(o.value("status").toString()));
        }
    });
    connect(service_, &AttendanceService::attendanceHistoryReceived, this, [this](const QJsonArray& arr) {
        if (!tblAttendance_) {
            historyUserTriggered_ = false;
            return;
        }
        if (arr.isEmpty() && historyUserTriggered_) {
            QMessageBox::warning(this, tr("失败"), tr("查询历史打卡失败"));
            historyUserTriggered_ = false;
            return;
        }
        tblAttendance_->setRowCount(0);
        tblAttendance_->setRowCount(arr.size());
        for (int i = 0; i < arr.size(); ++i) {
            const auto o = arr[i].toObject();
            tblAttendance_->setItem(i, 0, new QTableWidgetItem(o.value("checkin_date").toString()));
            tblAttendance_->setItem(i, 1, new QTableWidgetItem(o.value("checkin_time").toString()));
            tblAttendance_->setItem(i, 2, new QTableWidgetItem(o.value("created_at").toString()));
        }
        if (historyUserTriggered_)
            QMessageBox::information(this, tr("提示"), tr("查询历史打卡成功"));
        historyUserTriggered_ = false;
    });
    connect(service_, &AttendanceService::cancelLeaveResult, this, [this](bool /*ok*/, const QString& /*msg*/) { refreshActiveLeaves(); });
}

void AttendanceWidget::showCheckIn() { stack_->setCurrentIndex(0); if (btnCheckIn_) btnCheckIn_->setChecked(true); }
void AttendanceWidget::showLeave() { stack_->setCurrentIndex(1); if (btnLeave_) btnLeave_->setChecked(true); }
void AttendanceWidget::showCancelLeave() { stack_->setCurrentIndex(2); if (btnCancel_) btnCancel_->setChecked(true); }

void AttendanceWidget::submitLeave()
{
    if (service_)
        service_->submitLeave(doctorName_, leLeaveDate_ ? leLeaveDate_->text() : QDate::currentDate().toString("yyyy-MM-dd"), teReason_ ? teReason_->toPlainText() : QString());
}

void AttendanceWidget::refreshActiveLeaves()
{
    if (service_)
        service_->getActiveLeaves(doctorName_);
}

void AttendanceWidget::refreshAttendanceHistory()
{
    if (service_)
        service_->getAttendanceHistory(doctorName_);
}

void AttendanceWidget::cancelSelectedLeave()
{
    int row = tblLeaves_ ? tblLeaves_->currentRow() : -1;
    if (row < 0)
        return;
    int id = tblLeaves_->item(row, 0)->text().toInt();
    if (service_)
        service_->cancelLeave(id);
}

void AttendanceWidget::onConnected()
{
    // 初始查询一次活跃请假
    refreshActiveLeaves();
    // 同时拉取历史打卡
    refreshAttendanceHistory();
}

// 已服务化，不再直连客户端
