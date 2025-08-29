#include "diagnosiswidget.h"
#include <QVBoxLayout>
#include <QLabel>

DiagnosisWidget::DiagnosisWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("诊断/医嘱 - %1").arg(doctorName_)));
    setLayout(layout);
}
