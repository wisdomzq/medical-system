#include "checkinwidget.h"
#include <QVBoxLayout>
#include <QLabel>

CheckInWidget::CheckInWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("日常打卡 - %1").arg(doctorName_)));
    setLayout(layout);
}
