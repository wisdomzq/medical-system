#include "profilewidget.h"
#include <QVBoxLayout>
#include <QLabel>

ProfileWidget::ProfileWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("个人信息 - %1").arg(doctorName_)));
    setLayout(layout);
}
