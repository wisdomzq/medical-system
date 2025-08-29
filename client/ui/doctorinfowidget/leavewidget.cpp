#include "leavewidget.h"
#include <QVBoxLayout>
#include <QLabel>

LeaveWidget::LeaveWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("请假 - %1").arg(doctorName_)));
    setLayout(layout);
}
