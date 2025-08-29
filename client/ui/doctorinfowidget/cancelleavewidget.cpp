#include "cancelleavewidget.h"
#include <QVBoxLayout>
#include <QLabel>

CancelLeaveWidget::CancelLeaveWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("销假 - %1").arg(doctorName_)));
    setLayout(layout);
}
