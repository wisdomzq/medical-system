#ifndef DOCTORINFOWIDGET_H
#define DOCTORINFOWIDGET_H

#include <QWidget>

class QTabWidget;

// 医生端主界面容器：聚合多个业务模块为 Tab
class DoctorInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorInfoWidget(const QString &doctorName, QWidget *parent = nullptr);
    ~DoctorInfoWidget();

signals:
    void backToLogin();

private:
    QTabWidget *tabWidget {nullptr};
    QString m_doctorName;
};

#endif // DOCTORINFOWIDGET_H
