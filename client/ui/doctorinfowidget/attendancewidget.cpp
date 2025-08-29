#include "attendancewidget.h"
#include <QVBoxLayout>
#include <QLabel>

AttendanceWidget::AttendanceWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("考勤 - %1").arg(doctorName_)));
    setLayout(layout);
}
