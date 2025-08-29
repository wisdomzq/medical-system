#ifndef APPOINTMENTSWIDGET_H
#define APPOINTMENTSWIDGET_H

#include <QWidget>
#include <QString>

class AppointmentsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentsWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // APPOINTMENTSWIDGET_H
