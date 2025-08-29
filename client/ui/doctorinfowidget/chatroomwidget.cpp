#include "chatroomwidget.h"
#include <QVBoxLayout>
#include <QLabel>

ChatRoomWidget::ChatRoomWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("聊天室 - %1").arg(doctorName_)));
    setLayout(layout);
}
