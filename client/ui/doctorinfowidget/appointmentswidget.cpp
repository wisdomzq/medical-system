#include "appointmentswidget.h"
#include <QVBoxLayout>
#include <QLabel>

AppointmentsWidget::AppointmentsWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("预约信息 - %1").arg(doctorName_)));
    setLayout(layout);
}
