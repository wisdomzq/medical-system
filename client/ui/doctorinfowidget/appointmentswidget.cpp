#include "appointmentswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QAbstractItemView>
#include <QPushButton>
#include <QJsonArray>
#include <QDialog>
#include <QJsonObject>
#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QDate>
#include <QTimer>
#include <QDebug>
#include "appointmentdetailsdialog.h"
#include "core/network/src/client/communicationclient.h"
#include "core/network/src/protocol.h"

AppointmentsWidget::AppointmentsWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), ownsClient_(true) {
    client_ = new CommunicationClient(this);
    setupUI();
    
    connect(client_, &CommunicationClient::connected, this, &AppointmentsWidget::onConnected);
    connect(client_, &CommunicationClient::jsonReceived, this, &AppointmentsWidget::onJsonReceived);
    connect(refreshBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onRefresh);

    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
}

AppointmentsWidget::AppointmentsWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), client_(client), ownsClient_(false) {
    setupUI();
    
    // 注意：不连接jsonReceived信号，因为消息由主界面分发
    connect(refreshBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onRefresh);

    // 延迟请求数据，确保UI已经完全初始化
    QTimer::singleShot(500, this, &AppointmentsWidget::requestAppointments);
}

void AppointmentsWidget::setupUI() {
    auto* root = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout();
    refreshBtn_ = new QPushButton(tr("刷新"), this);
    auto* filterBtn = new QPushButton(tr("筛选"), this);
    auto* exportBtn = new QPushButton(tr("导出"), this);
    auto* statsBtn = new QPushButton(tr("统计"), this);
    
    topBar->addWidget(new QLabel(tr("医生预约管理")));
    topBar->addStretch();
    topBar->addWidget(filterBtn);
    topBar->addWidget(exportBtn);
    topBar->addWidget(statsBtn);
    topBar->addWidget(refreshBtn_);
    root->addLayout(topBar);
    
    // 添加统计信息区域
    auto* statsFrame = new QFrame(this);
    statsFrame->setFrameStyle(QFrame::StyledPanel);
    statsFrame->setStyleSheet("QFrame { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 5px; margin: 5px; }");
    statsFrame->setMinimumHeight(60);
    
    auto* statsLayout = new QHBoxLayout(statsFrame);
    auto* todayLabel = new QLabel(tr("今日预约: 0"), this);
    auto* pendingLabel = new QLabel(tr("待确认: 0"), this);
    auto* confirmedLabel = new QLabel(tr("已确认: 0"), this);
    auto* totalLabel = new QLabel(tr("总计: 0"), this);
    
    todayLabel->setObjectName("todayLabel");
    pendingLabel->setObjectName("pendingLabel"); 
    confirmedLabel->setObjectName("confirmedLabel");
    totalLabel->setObjectName("totalLabel");
    
    statsLayout->addWidget(todayLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(pendingLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(confirmedLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(totalLabel);
    
    root->addWidget(statsFrame);

    table_ = new QTableWidget(this);
    table_->setColumnCount(8);
    QStringList headers{tr("预约ID"), tr("患者姓名"), tr("预约日期"), tr("预约时间"), tr("科室"), tr("状态"), tr("费用"), tr("操作")};
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // 患者姓名
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // 预约日期
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 预约时间
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 科室
    table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 状态
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents); // 费用
    table_->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents); // 操作
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(table_);
}

void AppointmentsWidget::onConnected() {
    requestAppointments();
}

void AppointmentsWidget::onRefresh() {
    requestAppointments();
}

void AppointmentsWidget::requestAppointments() {
    QJsonObject req{
        {"action", "get_appointments_by_doctor"},
        {"username", doctorName_}
    };
    qDebug() << "[AppointmentsWidget] 发送请求:" << req;
    qDebug() << "[AppointmentsWidget] 医生用户名:" << doctorName_;
    qDebug() << "[AppointmentsWidget] 请求详情 - action:" << req.value("action").toString() << ", username:" << req.value("username").toString();
    client_->sendJson(req);
}

void AppointmentsWidget::onJsonReceived(const QJsonObject& obj) {
    qDebug() << "[AppointmentsWidget] 收到JSON响应:" << obj;
    const auto type = obj.value("type").toString();
    if (type == "appointments_response") {
        const auto success = obj.value("success").toBool();
        qDebug() << "[AppointmentsWidget] 预约响应成功:" << success;
        if (!success) {
            qDebug() << "[AppointmentsWidget] 预约查询失败:" << obj.value("error").toString();
            return;
        }
        const auto arr = obj.value("data").toArray();
        qDebug() << "[AppointmentsWidget] 预约数据数量:" << arr.size();
        table_->setRowCount(arr.size());
        
        // 初始化统计计数器
        int totalCount = arr.size();
        int todayCount = 0;
        int pendingCount = 0;
        int confirmedCount = 0;
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        
        int row = 0;
        for (const auto& v : arr) {
            const auto appt = v.toObject();
            const auto appointmentId = appt.value("id").toInt();
            const auto patientName = appt.value("patient_name").toString();
            const auto date = appt.value("appointment_date").toString();
            const auto time = appt.value("appointment_time").toString();
            const auto department = appt.value("department").toString();
            const auto status = appt.value("status").toString();
            const auto fee = appt.value("fee").toDouble();
            
            // 统计数据
            if (date == today) {
                todayCount++;
            }
            if (status == "pending") {
                pendingCount++;
            } else if (status == "confirmed") {
                confirmedCount++;
            }
            
            auto* idItem = new QTableWidgetItem(QString::number(appointmentId));
            auto* nameItem = new QTableWidgetItem(patientName);
            auto* dateItem = new QTableWidgetItem(date);
            auto* timeItem = new QTableWidgetItem(time);
            auto* deptItem = new QTableWidgetItem(department);
            auto* statusItem = new QTableWidgetItem(status);
            auto* feeItem = new QTableWidgetItem(QString::number(fee, 'f', 2));
            
            // 设置状态项的颜色
            if (status == "confirmed") {
                statusItem->setBackground(QBrush(QColor(144, 238, 144))); // 浅绿色
            } else if (status == "pending") {
                statusItem->setBackground(QBrush(QColor(255, 255, 224))); // 浅黄色
            } else if (status == "cancelled") {
                statusItem->setBackground(QBrush(QColor(255, 182, 193))); // 浅红色
            }
            
            // 添加更丰富的患者信息工具提示
            QString patientTooltip = QString("患者: %1").arg(patientName);
            if (!appt.value("patient_age").toString().isEmpty()) {
                patientTooltip += QString("\n年龄: %1").arg(appt.value("patient_age").toInt());
            }
            if (!appt.value("patient_gender").toString().isEmpty()) {
                patientTooltip += QString("\n性别: %1").arg(appt.value("patient_gender").toString());
            }
            if (!appt.value("patient_phone").toString().isEmpty()) {
                patientTooltip += QString("\n电话: %1").arg(appt.value("patient_phone").toString());
            }
            if (!appt.value("chief_complaint").toString().isEmpty()) {
                patientTooltip += QString("\n主诉: %1").arg(appt.value("chief_complaint").toString());
            }
            nameItem->setToolTip(patientTooltip);
            
            // 添加排班信息工具提示
            QString scheduleTooltip = QString("预约时间: %1 %2").arg(date, time);
            if (!appt.value("schedule_start_time").toString().isEmpty()) {
                scheduleTooltip += QString("\n工作时间: %1 - %2")
                    .arg(appt.value("schedule_start_time").toString())
                    .arg(appt.value("schedule_end_time").toString());
            }
            if (appt.value("schedule_max_appointments").toInt() > 0) {
                scheduleTooltip += QString("\n当日最大预约数: %1")
                    .arg(appt.value("schedule_max_appointments").toInt());
            }
            dateItem->setToolTip(scheduleTooltip);
            timeItem->setToolTip(scheduleTooltip);
            
            table_->setItem(row, 0, idItem);
            table_->setItem(row, 1, nameItem);
            table_->setItem(row, 2, dateItem);
            table_->setItem(row, 3, timeItem);
            table_->setItem(row, 4, deptItem);
            table_->setItem(row, 5, statusItem);
            table_->setItem(row, 6, feeItem);
            
            auto* btn = new QPushButton(tr("详情"), table_);
            btn->setProperty("appt", appt); // store full appt json
            connect(btn, &QPushButton::clicked, this, &AppointmentsWidget::onRowDetailClicked);
            table_->setCellWidget(row, 7, btn);
            ++row;
        }
        
        // 更新统计标签
        auto* todayLabel = findChild<QLabel*>("todayLabel");
        auto* pendingLabel = findChild<QLabel*>("pendingLabel");
        auto* confirmedLabel = findChild<QLabel*>("confirmedLabel");
        auto* totalLabel = findChild<QLabel*>("totalLabel");
        
        if (todayLabel) todayLabel->setText(tr("今日预约: %1").arg(todayCount));
        if (pendingLabel) pendingLabel->setText(tr("待确认: %1").arg(pendingCount));
        if (confirmedLabel) confirmedLabel->setText(tr("已确认: %1").arg(confirmedCount));
        if (totalLabel) totalLabel->setText(tr("总计: %1").arg(totalCount));
    }
}

void AppointmentsWidget::onRowDetailClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    const auto appt = btn->property("appt").toJsonObject();
    openDetailDialog(appt);
}

void AppointmentsWidget::openDetailDialog(const QJsonObject& appt) {
    AppointmentDetailsDialog dlg(doctorName_, appt, this);
    dlg.exec();
}
