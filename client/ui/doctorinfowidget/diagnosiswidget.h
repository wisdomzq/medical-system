#ifndef DIAGNOSISWIDGET_H
#define DIAGNOSISWIDGET_H

#include <QWidget>
#include <QString>

class DiagnosisWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosisWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // DIAGNOSISWIDGET_H
