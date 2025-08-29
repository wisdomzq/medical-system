#ifndef LEAVEWIDGET_H
#define LEAVEWIDGET_H

#include <QWidget>
#include <QString>

class LeaveWidget : public QWidget {
    Q_OBJECT
public:
    explicit LeaveWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // LEAVEWIDGET_H
