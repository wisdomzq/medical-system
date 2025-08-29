#ifndef ATTENDANCEWIDGET_H
#define ATTENDANCEWIDGET_H

#include <QWidget>
#include <QString>

class AttendanceWidget : public QWidget {
    Q_OBJECT
public:
    explicit AttendanceWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // ATTENDANCEWIDGET_H
