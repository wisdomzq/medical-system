#include "attendancewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QDate>
#include <QTime>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include "core/network/src/client/communicationclient.h"
#include "core/network/src/protocol.h"

AttendanceWidget::AttendanceWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* root = new QVBoxLayout(this);

    // 顶部导航按钮
    auto* nav = new QHBoxLayout();
    btnCheckIn_ = new QPushButton(tr("日常打卡"), this);
    btnLeave_ = new QPushButton(tr("请假"), this);
    btnCancel_ = new QPushButton(tr("销假"), this);
    nav->addStretch();
    nav->addWidget(btnCheckIn_);
    nav->addWidget(btnLeave_);
    nav->addWidget(btnCancel_);
    root->addLayout(nav);

    // 内容堆栈页
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
    row->addWidget(lbDate); row->addWidget(leDate); row->addWidget(lbTime); row->addWidget(leTime); row->addWidget(btnDoCheckIn_); row->addWidget(btnHistory_);
        l->addLayout(row);
    // 历史表格
    tblAttendance_ = new QTableWidget(0, 3, pageCheckIn);
    tblAttendance_->setHorizontalHeaderLabels({tr("日期"), tr("时间"), tr("创建时间")});
    tblAttendance_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    l->addWidget(tblAttendance_);
        l->addStretch();

        connect(btnDoCheckIn_, &QPushButton::clicked, this, [this, leDate, leTime]{
            QJsonObject req{{"action","doctor_checkin"}, {"doctor_username", doctorName_}, {"checkin_date", leDate->text()}, {"checkin_time", leTime->text()}};
            if (client_) client_->sendJson(req);
        });
    connect(btnHistory_, &QPushButton::clicked, this, [this]{ historyUserTriggered_ = true; refreshAttendanceHistory(); });
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
        tblLeaves_->setHorizontalHeaderLabels({tr("ID"), tr("日期"), tr("原因"), tr("状态")});
        tblLeaves_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        l->addWidget(tblLeaves_);
        l->addStretch();

        connect(btnRefreshLeaves_, &QPushButton::clicked, this, &AttendanceWidget::refreshActiveLeaves);
        connect(btnCancelLeave_, &QPushButton::clicked, this, &AttendanceWidget::cancelSelectedLeave);
    }
    stack_->addWidget(pageCheckIn);   // index 0
    stack_->addWidget(pageLeave);     // index 1
    stack_->addWidget(pageCancel);    // index 2
    root->addWidget(stack_);

    // 连接信号
    connect(btnCheckIn_, &QPushButton::clicked, this, &AttendanceWidget::showCheckIn);
    connect(btnLeave_, &QPushButton::clicked, this, &AttendanceWidget::showLeave);
    connect(btnCancel_, &QPushButton::clicked, this, &AttendanceWidget::showCancelLeave);

    // 默认显示打卡页
    showCheckIn();

    // 建立网络连接
    client_ = new CommunicationClient(this);
    connect(client_, &CommunicationClient::connected, this, &AttendanceWidget::onConnected);
    connect(client_, &CommunicationClient::jsonReceived, this, &AttendanceWidget::onJsonReceived);
    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
}

void AttendanceWidget::showCheckIn() { stack_->setCurrentIndex(0); }
void AttendanceWidget::showLeave() { stack_->setCurrentIndex(1); }
void AttendanceWidget::showCancelLeave() { stack_->setCurrentIndex(2); }

void AttendanceWidget::doCheckIn() {
    QJsonObject req{{"action","doctor_checkin"}, {"doctor_username", doctorName_}};
    if (client_) client_->sendJson(req);
}

void AttendanceWidget::submitLeave() {
    QJsonObject req{{"action","doctor_leave"}, {"doctor_username", doctorName_}, {"leave_date", leLeaveDate_? leLeaveDate_->text() : QDate::currentDate().toString("yyyy-MM-dd")}, {"reason", teReason_? teReason_->toPlainText():QString()}};
    if (client_) client_->sendJson(req);
}

void AttendanceWidget::refreshActiveLeaves() {
    QJsonObject req{{"action","get_active_leaves"}, {"doctor_username", doctorName_}};
    if (client_) client_->sendJson(req);
}

void AttendanceWidget::refreshAttendanceHistory() {
    QJsonObject req{{"action","get_attendance_history"}, {"doctor_username", doctorName_}};
    if (client_) client_->sendJson(req);
}

void AttendanceWidget::cancelSelectedLeave() {
    int row = tblLeaves_? tblLeaves_->currentRow() : -1;
    if (row < 0) return;
    int id = tblLeaves_->item(row, 0)->text().toInt();
    QJsonObject req{{"action","cancel_leave"}, {"leave_id", id}};
    if (client_) client_->sendJson(req);
}

void AttendanceWidget::onConnected() {
    // 初始查询一次活跃请假
    refreshActiveLeaves();
    // 同时拉取历史打卡
    refreshAttendanceHistory();
}

void AttendanceWidget::onJsonReceived(const QJsonObject& obj) {
    const QString type = obj.value("type").toString();
    if (type == "doctor_checkin_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) {
            // 提示并刷新历史列表，确保新纪录可见
            QMessageBox::information(this, tr("提示"), tr("打卡成功"));
            refreshAttendanceHistory();
        } else {
            QMessageBox::warning(this, tr("失败"), tr("打卡失败"));
        }
    } else if (type == "doctor_leave_response") {
        // 成功后切到销假页以便查看/撤销，同时直接在表格中附加一行，避免依赖刷新延迟
        const bool ok = obj.value("success").toBool();
        showCancelLeave();
        if (ok) {
            const QJsonObject d = obj.value("data").toObject();
            // 直接追加一行（id 可能未知，这里用空或 - ）
            int row = tblLeaves_->rowCount();
            tblLeaves_->insertRow(row);
            tblLeaves_->setItem(row, 0, new QTableWidgetItem("-"));
            tblLeaves_->setItem(row, 1, new QTableWidgetItem(d.value("leave_date").toString()));
            tblLeaves_->setItem(row, 2, new QTableWidgetItem(d.value("reason").toString()));
            tblLeaves_->setItem(row, 3, new QTableWidgetItem("active"));
        }
        // 再发起一次刷新以拿到真实ID
        refreshActiveLeaves();
    } else if (type == "active_leaves_response") {
        tblLeaves_->setRowCount(0);
        if (!obj.value("success").toBool()) return;
        const QJsonArray arr = obj.value("data").toArray();
        tblLeaves_->setRowCount(arr.size());
        for (int i=0;i<arr.size();++i) {
            const auto o = arr[i].toObject();
            tblLeaves_->setItem(i, 0, new QTableWidgetItem(QString::number(o.value("id").toInt())));
            tblLeaves_->setItem(i, 1, new QTableWidgetItem(o.value("leave_date").toString()));
            tblLeaves_->setItem(i, 2, new QTableWidgetItem(o.value("reason").toString()));
            tblLeaves_->setItem(i, 3, new QTableWidgetItem(o.value("status").toString()));
        }
    } else if (type == "attendance_history_response") {
        if (!tblAttendance_) { historyUserTriggered_ = false; return; }
        const bool ok = obj.value("success").toBool();
        if (!ok) {
            if (historyUserTriggered_) {
                const QString msg = obj.value("message").toString();
                QMessageBox::warning(this, tr("失败"), msg.isEmpty() ? tr("查询历史打卡失败") : msg);
            }
            historyUserTriggered_ = false;
            return;
        }
        // 成功时再刷新表格
        const QJsonArray arr = obj.value("data").toArray();
        tblAttendance_->setRowCount(0);
        tblAttendance_->setRowCount(arr.size());
        for (int i=0;i<arr.size();++i) {
            const auto o = arr[i].toObject();
            tblAttendance_->setItem(i, 0, new QTableWidgetItem(o.value("checkin_date").toString()));
            tblAttendance_->setItem(i, 1, new QTableWidgetItem(o.value("checkin_time").toString()));
            tblAttendance_->setItem(i, 2, new QTableWidgetItem(o.value("created_at").toString()));
        }
        if (historyUserTriggered_) {
            QMessageBox::information(this, tr("提示"), tr("查询历史打卡成功"));
        }
        historyUserTriggered_ = false;
    } else if (type == "cancel_leave_response") {
        refreshActiveLeaves();
    }
}
