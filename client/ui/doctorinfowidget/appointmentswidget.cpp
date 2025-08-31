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
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include "appointmentdetailsdialog.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/appointmentservice.h"

AppointmentsWidget::AppointmentsWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), client_(client), ownsClient_(false) {
    setupUI();
    
    // 不再依赖外部分发：使用服务自己监听
    connect(refreshBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onRefresh);

    service_ = new AppointmentService(client_, this);
    connect(service_, &AppointmentService::fetched, this, &AppointmentsWidget::renderAppointments);
    connect(service_, &AppointmentService::fetchFailed, this, &AppointmentsWidget::showFetchError);

    QTimer::singleShot(500, this, &AppointmentsWidget::requestAppointments);
}

void AppointmentsWidget::setupUI() {
    auto* root = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout();
    refreshBtn_ = new QPushButton(tr("刷新"), this);
    filterBtn_ = new QPushButton(tr("筛选"), this);
    exportBtn_ = new QPushButton(tr("导出"), this);
    statsBtn_ = new QPushButton(tr("统计"), this);
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("输入患者姓名进行筛选"));
    filterEdit_->setMaximumWidth(200);
    
    topBar->addWidget(new QLabel(tr("医生预约管理")));
    topBar->addStretch();
    topBar->addWidget(filterEdit_);
    topBar->addWidget(filterBtn_);
    topBar->addWidget(exportBtn_);
    topBar->addWidget(statsBtn_);
    topBar->addWidget(refreshBtn_);
    root->addLayout(topBar);
    
    // 连接按钮事件
    connect(filterBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onFilterClicked);
    connect(exportBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onExportClicked);
    connect(statsBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onStatsClicked);
    
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
        else if (status == "confirmed") confirmedCount++;
        else if (status == "completed") confirmedCount++; // completed也算已确认

        auto* idItem = new QTableWidgetItem(QString::number(appointmentId));
        auto* nameItem = new QTableWidgetItem(patientName);
        auto* dateItem = new QTableWidgetItem(date);
        auto* timeItem = new QTableWidgetItem(time);
        auto* deptItem = new QTableWidgetItem(department);
        auto* statusItem = new QTableWidgetItem(status);
        auto* feeItem = new QTableWidgetItem(QString::number(fee, 'f', 2));

        if (status == "confirmed") statusItem->setBackground(QBrush(QColor(144, 238, 144)));
        else if (status == "completed") statusItem->setBackground(QBrush(QColor(173, 255, 47))); // 绿黄色表示已完成
        else if (status == "pending") statusItem->setBackground(QBrush(QColor(255, 255, 224)));
        else if (status == "cancelled") statusItem->setBackground(QBrush(QColor(255, 182, 193)));

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

        auto* btn = new QPushButton(tr("详情"), table_);
        btn->setProperty("appt", appt);
        connect(btn, &QPushButton::clicked, this, &AppointmentsWidget::onRowDetailClicked);
        table_->setCellWidget(row, 7, btn);
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
    dlg.exec();
}

void AppointmentsWidget::onFilterClicked() {
    QString filterText = filterEdit_->text().trimmed();
    if (filterText.isEmpty()) {
        // 显示所有行
        for (int row = 0; row < table_->rowCount(); ++row) {
            table_->setRowHidden(row, false);
        }
        return;
    }
    
    // 根据患者姓名筛选现有数据
    for (int row = 0; row < table_->rowCount(); ++row) {
        QTableWidgetItem* nameItem = table_->item(row, 1);
        if (nameItem) {
            bool visible = nameItem->text().contains(filterText, Qt::CaseInsensitive);
            table_->setRowHidden(row, !visible);
        }
    }
}

void AppointmentsWidget::onExportClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "导出预约数据", 
                                                   QString("appointments_%1.csv").arg(QDate::currentDate().toString("yyyy-MM-dd")),
                                                   "CSV Files (*.csv)");
    if (!fileName.isEmpty()) {
        exportToCSV(fileName);
    }
}

void AppointmentsWidget::onStatsClicked() {
    auto* todayLabel = findChild<QLabel*>("todayLabel");
    auto* pendingLabel = findChild<QLabel*>("pendingLabel");
    auto* confirmedLabel = findChild<QLabel*>("confirmedLabel");
    auto* totalLabel = findChild<QLabel*>("totalLabel");
    
    QString stats = QString("预约统计信息:\n\n%1\n%2\n%3\n%4")
                    .arg(todayLabel ? todayLabel->text() : "今日预约: 0")
                    .arg(pendingLabel ? pendingLabel->text() : "待确认: 0")
                    .arg(confirmedLabel ? confirmedLabel->text() : "已确认: 0")
                    .arg(totalLabel ? totalLabel->text() : "总计: 0");
    
    QMessageBox::information(this, "预约统计", stats);
}

void AppointmentsWidget::exportToCSV(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法创建文件: " + fileName);
        return;
    }
    
    QTextStream out(&file);
    out << "预约ID,患者姓名,预约日期,预约时间,科室,状态,费用\n";
    
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (table_->isRowHidden(row)) continue;
        
        QStringList rowData;
        for (int col = 0; col < 7; ++col) { // 不包括操作列
            QTableWidgetItem* item = table_->item(row, col);
            rowData << (item ? item->text() : "");
        }
        out << rowData.join(",") << "\n";
    }
    
    QMessageBox::information(this, "导出成功", "预约数据已导出到: " + fileName);
}
