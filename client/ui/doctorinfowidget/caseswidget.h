#ifndef CASESWIDGET_H
#define CASESWIDGET_H

#include <QWidget>
#include <QString>

class CasesWidget : public QWidget {
    Q_OBJECT
public:
    explicit CasesWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // CASESWIDGET_H
