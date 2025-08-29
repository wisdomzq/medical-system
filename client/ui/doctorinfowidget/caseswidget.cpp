#include "caseswidget.h"
#include <QVBoxLayout>
#include <QLabel>

CasesWidget::CasesWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("病例详情 - %1").arg(doctorName_)));
    setLayout(layout);
}
