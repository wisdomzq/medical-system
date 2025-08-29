#ifndef CANCELLEAVEWIDGET_H
#define CANCELLEAVEWIDGET_H

#include <QWidget>
#include <QString>

class CancelLeaveWidget : public QWidget {
    Q_OBJECT
public:
    explicit CancelLeaveWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // CANCELLEAVEWIDGET_H
