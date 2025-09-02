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
#include <QRegExp>
#include "appointmentdetailsdialog.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/appointmentservice.h"

AppointmentsWidget::AppointmentsWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), client_(client), ownsClient_(false) {
    setupUI();
    // 加载专用样式
    QFile f(":/doctor_appointment.qss");
    if (f.open(QIODevice::ReadOnly)) setStyleSheet(QString::fromUtf8(f.readAll()));
    
    // 不再依赖外部分发：使用服务自己监听
    connect(refreshBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onRefresh);

    service_ = new AppointmentService(client_, this);
    connect(service_, &AppointmentService::fetched, this, &AppointmentsWidget::renderAppointments);
    connect(service_, &AppointmentService::fetchFailed, this, &AppointmentsWidget::showFetchError);

    QTimer::singleShot(500, this, &AppointmentsWidget::requestAppointments);
}

void AppointmentsWidget::setupUI() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(12);

    // 顶部条
    auto* topBarW = new QWidget(this);
    topBarW->setObjectName("apptTopBar");
    topBarW->setAttribute(Qt::WA_StyledBackground, true);
    auto* topBar = new QHBoxLayout(topBarW);
    topBar->setContentsMargins(12,8,12,8);
    auto* title = new QLabel(tr("医生预约管理"), topBarW);
    title->setObjectName("apptTitle");
    refreshBtn_ = new QPushButton(tr("刷新"), this);
    auto* filterBtn = new QPushButton(tr("筛选"), this);
    auto* exportBtn = new QPushButton(tr("导出"), this);
    auto* statsBtn = new QPushButton(tr("统计"), this);
    // 标记为次按钮风格
    for (auto* b : {filterBtn, exportBtn, statsBtn, refreshBtn_}) b->setProperty("class", "ApptSecondary");
    topBar->addWidget(title);
    topBar->addStretch();
    topBar->addWidget(filterBtn);
    topBar->addWidget(exportBtn);
    topBar->addWidget(statsBtn);
    topBar->addWidget(refreshBtn_);
    root->addWidget(topBarW);
    
    // 添加统计信息区域
    auto* statsFrame = new QFrame(this);
    statsFrame->setObjectName("statsCard");
    statsFrame->setFrameStyle(QFrame::NoFrame);
    statsFrame->setMinimumHeight(60);
    
    auto* statsLayout = new QHBoxLayout(statsFrame);
    statsLayout->setContentsMargins(16, 12, 16, 12);
    statsLayout->setSpacing(12);
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
    table_->setObjectName("apptTable");
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
    table_->setAlternatingRowColors(false); // 禁用交替行颜色
    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(36);
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    // 表格外层卡片容器
    auto* tableCard = new QWidget(this);
    tableCard->setObjectName("apptTableCard");
    tableCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* tableCardLy = new QVBoxLayout(tableCard);
    tableCardLy->setContentsMargins(12, 12, 12, 12);
    tableCardLy->setSpacing(0);
    tableCardLy->addWidget(table_);
    root->addWidget(tableCard);
}

void AppointmentsWidget::onConnected() {
    requestAppointments();
}

void AppointmentsWidget::onRefresh() {
    requestAppointments();
}

void AppointmentsWidget::requestAppointments() {
    service_->fetchByDoctor(doctorName_);
}

void AppointmentsWidget::renderAppointments(const QJsonArray& arr) {
    qDebug() << "[AppointmentsWidget] 预约数据数量:" << arr.size();
    table_->setRowCount(arr.size());
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

    if (date == today) todayCount++;
    if (status == "pending") pendingCount++; 
    else if (status == "completed") confirmedCount++;

    auto* idItem = new QTableWidgetItem(QString::number(appointmentId));
    auto* nameItem = new QTableWidgetItem(patientName);
    auto* dateItem = new QTableWidgetItem(date);
    auto* timeItem = new QTableWidgetItem(time);
    auto* deptItem = new QTableWidgetItem(department);
    auto* statusItem = new QTableWidgetItem(status);
    auto* feeItem = new QTableWidgetItem(QString::number(fee, 'f', 2));
    // 居中显示
    idItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setTextAlignment(Qt::AlignCenter);
    dateItem->setTextAlignment(Qt::AlignCenter);
    timeItem->setTextAlignment(Qt::AlignCenter);
    deptItem->setTextAlignment(Qt::AlignCenter);
    statusItem->setTextAlignment(Qt::AlignCenter);
    feeItem->setTextAlignment(Qt::AlignCenter);

    // 只对状态列设置背景色：pending黄色，completed绿色
    if (status == "pending") {
        statusItem->setBackground(QBrush(QColor(255, 255, 224))); // 黄色背景
    } else if (status == "completed") {
        statusItem->setBackground(QBrush(QColor(144, 238, 144))); // 绿色背景
    }

        QString patientTooltip = QString("患者: %1").arg(patientName);
        if (!appt.value("patient_age").toString().isEmpty()) patientTooltip += QString("\n年龄: %1").arg(appt.value("patient_age").toInt());
        if (!appt.value("patient_gender").toString().isEmpty()) patientTooltip += QString("\n性别: %1").arg(appt.value("patient_gender").toString());
        if (!appt.value("patient_phone").toString().isEmpty()) patientTooltip += QString("\n电话: %1").arg(appt.value("patient_phone").toString());
        if (!appt.value("chief_complaint").toString().isEmpty()) patientTooltip += QString("\n主诉: %1").arg(appt.value("chief_complaint").toString());
        nameItem->setToolTip(patientTooltip);

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

    // 操作列：按钮居中
    auto* btn = new QPushButton(tr("详情"), table_);
    btn->setProperty("appt", appt);
    connect(btn, &QPushButton::clicked, this, &AppointmentsWidget::onRowDetailClicked);
    auto* actionCell = new QWidget(table_);
    auto* actionLy = new QHBoxLayout(actionCell);
    actionLy->setContentsMargins(0,0,0,0);
    actionLy->setSpacing(0);
    actionLy->addStretch();
    actionLy->addWidget(btn);
    actionLy->addStretch();
    table_->setCellWidget(row, 7, actionCell);
        ++row;
    }

    auto* todayLabel = findChild<QLabel*>("todayLabel");
    auto* pendingLabel = findChild<QLabel*>("pendingLabel");
    auto* confirmedLabel = findChild<QLabel*>("confirmedLabel");
    auto* totalLabel = findChild<QLabel*>("totalLabel");
    if (todayLabel) todayLabel->setText(tr("今日预约: %1").arg(todayCount));
    if (pendingLabel) pendingLabel->setText(tr("待确认: %1").arg(pendingCount));
    if (confirmedLabel) confirmedLabel->setText(tr("已确认: %1").arg(confirmedCount));
    if (totalLabel) totalLabel->setText(tr("总计: %1").arg(totalCount));
}

void AppointmentsWidget::showFetchError(const QString& message) {
    qDebug() << "[AppointmentsWidget] 预约查询失败:" << message;
}

void AppointmentsWidget::onRowDetailClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    const auto appt = btn->property("appt").toJsonObject();
    openDetailDialog(appt);
}

void AppointmentsWidget::openDetailDialog(const QJsonObject& appt) {
    AppointmentDetailsDialog dlg(doctorName_, appt, client_, this);
    connect(&dlg, &AppointmentDetailsDialog::diagnosisCompleted, this, &AppointmentsWidget::onDiagnosisCompleted);
    if (dlg.exec() == QDialog::Accepted) {
        // 诊断完成后刷新
        requestAppointments();
    }
}

void AppointmentsWidget::onDiagnosisCompleted(int appointmentId) {
    // 1) 更新表格中该行的状态显示为 completed
    for (int r = 0; r < table_->rowCount(); ++r) {
        auto* idItem = table_->item(r, 0);
        if (!idItem) continue;
        if (idItem->text().toInt() == appointmentId) {
            auto* statusItem = table_->item(r, 5);
            if (statusItem) {
                statusItem->setText("completed");
                statusItem->setBackground(QBrush(QColor(144, 238, 144))); // completed绿色背景
            }
            break;
        }
    }
    // 2) 调整顶部计数：待确认-1，已确认+1
    adjustCountersForCompletion();
}

void AppointmentsWidget::adjustCountersForCompletion() {
    auto* pendingLabel = findChild<QLabel*>("pendingLabel");
    auto* confirmedLabel = findChild<QLabel*>("confirmedLabel");
    if (!pendingLabel || !confirmedLabel) return;
    // 从文本中提取数字
    auto extract = [](const QString& text) -> int {
        QRegExp rx("(\\d+)");
        if (rx.indexIn(text) != -1) return rx.cap(1).toInt();
        return 0;
    };
    int pending = extract(pendingLabel->text());
    int confirmed = extract(confirmedLabel->text());
    if (pending > 0) pending -= 1;
    confirmed += 1;
    pendingLabel->setText(tr("待确认: %1").arg(pending));
    confirmedLabel->setText(tr("已确认: %1").arg(confirmed));
}
