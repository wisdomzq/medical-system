#include "attendancewidget.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/attendanceservice.h"
#include <QDate>
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
#include <QVBoxLayout>

AttendanceWidget::AttendanceWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent)
    , doctorName_(doctorName)
    , client_(client)
{
    auto* root = new QVBoxLayout(this);

    auto* nav = new QHBoxLayout();
    btnCheckIn_ = new QPushButton(tr("日常打卡"), this);
    btnLeave_ = new QPushButton(tr("请假"), this);
    btnCancel_ = new QPushButton(tr("销假"), this);
    nav->addStretch();
    nav->addWidget(btnCheckIn_);
    nav->addWidget(btnLeave_);
    nav->addWidget(btnCancel_);
    root->addLayout(nav);

    stack_ = new QStackedWidget(this);
    auto* pageCheckIn = new QWidget(this);
    {
        auto* l = new QVBoxLayout(pageCheckIn);
        l->addWidget(new QLabel(QString("%1 - 日常打卡").arg(doctorName_), pageCheckIn));
        auto* row = new QHBoxLayout();
        auto* lbDate = new QLabel(tr("日期(yyyy-MM-dd):"), pageCheckIn);
        auto* leDate = new QLineEdit(QDate::currentDate().toString("yyyy-MM-dd"), pageCheckIn);
        auto* lbTime = new QLabel(tr("时间(HH:mm:ss):"), pageCheckIn);
        auto* leTime = new QLineEdit(QTime::currentTime().toString("HH:mm:ss"), pageCheckIn);
        btnDoCheckIn_ = new QPushButton(tr("打卡"), pageCheckIn);
        btnHistory_ = new QPushButton(tr("查看历史打卡"), pageCheckIn);
        row->addWidget(lbDate);
        row->addWidget(leDate);
        row->addWidget(lbTime);
        row->addWidget(leTime);
        row->addWidget(btnDoCheckIn_);
        row->addWidget(btnHistory_);
        l->addLayout(row);
        tblAttendance_ = new QTableWidget(0, 3, pageCheckIn);
        tblAttendance_->setHorizontalHeaderLabels({ tr("日期"), tr("时间"), tr("创建时间") });
        tblAttendance_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        l->addWidget(tblAttendance_);
        l->addStretch();

        connect(btnDoCheckIn_, &QPushButton::clicked, this, [this, leDate, leTime] {
            if (service_)
                service_->checkIn(doctorName_, leDate->text(), leTime->text());
        });
        connect(btnHistory_, &QPushButton::clicked, this, [this] { historyUserTriggered_ = true; refreshAttendanceHistory(); });
    }
    auto* pageLeave = new QWidget(this);
    {
        auto* l = new QVBoxLayout(pageLeave);
        l->addWidget(new QLabel(QString("%1 - 请假").arg(doctorName_), pageLeave));
        auto* row1 = new QHBoxLayout();
        row1->addWidget(new QLabel(tr("请假日期(yyyy-MM-dd):"), pageLeave));
        leLeaveDate_ = new QLineEdit(QDate::currentDate().toString("yyyy-MM-dd"), pageLeave);
        row1->addWidget(leLeaveDate_);
        l->addLayout(row1);
        teReason_ = new QTextEdit(pageLeave);
        teReason_->setPlaceholderText(tr("请假原因..."));
        l->addWidget(teReason_);
        btnSubmitLeave_ = new QPushButton(tr("保存请假"), pageLeave);
        l->addWidget(btnSubmitLeave_);
        l->addStretch();

        connect(btnSubmitLeave_, &QPushButton::clicked, this, &AttendanceWidget::submitLeave);
    }
    auto* pageCancel = new QWidget(this);
    {
        auto* l = new QVBoxLayout(pageCancel);
        l->addWidget(new QLabel(QString("%1 - 销假").arg(doctorName_), pageCancel));
        auto* bar = new QHBoxLayout();
        btnRefreshLeaves_ = new QPushButton(tr("刷新请假记录"), pageCancel);
        btnCancelLeave_ = new QPushButton(tr("销选中假"), pageCancel);
        bar->addWidget(btnRefreshLeaves_);
        bar->addWidget(btnCancelLeave_);
        bar->addStretch();
        l->addLayout(bar);
        tblLeaves_ = new QTableWidget(0, 4, pageCancel);
        tblLeaves_->setHorizontalHeaderLabels({ tr("ID"), tr("日期"), tr("原因"), tr("状态") });
        tblLeaves_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        l->addWidget(tblLeaves_);
        l->addStretch();

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

void AttendanceWidget::showCheckIn() { stack_->setCurrentIndex(0); }
void AttendanceWidget::showLeave() { stack_->setCurrentIndex(1); }
void AttendanceWidget::showCancelLeave() { stack_->setCurrentIndex(2); }

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
