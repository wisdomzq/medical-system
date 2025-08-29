#ifndef CHATROOMWIDGET_H
#define CHATROOMWIDGET_H

#include <QWidget>
#include <QString>

class ChatRoomWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatRoomWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // CHATROOMWIDGET_H
