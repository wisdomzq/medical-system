#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <QWidget>
#include <QString>

class ProfileWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProfileWidget(const QString& doctorName, QWidget* parent=nullptr);
private:
    QString doctorName_;
};

#endif // PROFILEWIDGET_H
