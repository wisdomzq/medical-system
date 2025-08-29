#ifndef CHECKINWIDGET_H
#define CHECKINWIDGET_H

#include <QWidget>
#include <QString>

class CheckInWidget : public QWidget {
    Q_OBJECT
public:
    explicit CheckInWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // CHECKINWIDGET_H
