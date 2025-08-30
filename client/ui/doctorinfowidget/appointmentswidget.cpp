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
#include "appointmentdetailsdialog.h"
#include "core/network/src/client/communicationclient.h"
#include "core/network/src/protocol.h"

AppointmentsWidget::AppointmentsWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* root = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout();
    refreshBtn_ = new QPushButton(tr("刷新"), this);
    topBar->addStretch();
    topBar->addWidget(refreshBtn_);
    root->addLayout(topBar);

    table_ = new QTableWidget(this);
    table_->setColumnCount(3);
    QStringList headers{tr("患者姓名"), tr("预约时间"), tr("操作")};
    table_->setHorizontalHeaderLabels(headers);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(table_);

    client_ = new CommunicationClient(this);
    connect(client_, &CommunicationClient::connected, this, &AppointmentsWidget::onConnected);
    connect(client_, &CommunicationClient::jsonReceived, this, &AppointmentsWidget::onJsonReceived);
    connect(refreshBtn_, &QPushButton::clicked, this, &AppointmentsWidget::onRefresh);

    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
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
    client_->sendJson(req);
}

void AppointmentsWidget::onJsonReceived(const QJsonObject& obj) {
    const auto type = obj.value("type").toString();
    if (type == "appointments_response") {
        const auto success = obj.value("success").toBool();
        if (!success) return;
        const auto arr = obj.value("data").toArray();
        table_->setRowCount(arr.size());
        int row = 0;
        for (const auto& v : arr) {
            const auto appt = v.toObject();
            const auto patientName = appt.value("patient_name").toString();
            const auto date = appt.value("appointment_date").toString();
            const auto time = appt.value("appointment_time").toString();
            const auto ts = date + " " + time;
            auto* nameItem = new QTableWidgetItem(patientName);
            auto* timeItem = new QTableWidgetItem(ts);
            table_->setItem(row, 0, nameItem);
            table_->setItem(row, 1, timeItem);
            auto* btn = new QPushButton(tr("详情"), table_);
            btn->setProperty("appt", appt); // store full appt json
            connect(btn, &QPushButton::clicked, this, &AppointmentsWidget::onRowDetailClicked);
            table_->setCellWidget(row, 2, btn);
            ++row;
        }
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
