#ifndef LOGINMODULE_H
#define LOGINMODULE_H

#include <QObject>
#include <QJsonObject>
class DBManager; // forward declaration

class LoginModule : public QObject {
    Q_OBJECT
public:
    explicit LoginModule(QObject *parent = nullptr);
    ~LoginModule();

public slots:
    QJsonObject handleLogin(const QJsonObject &request);
    QJsonObject handleRegister(const QJsonObject &request);

private:
    DBManager *m_db;
};

#endif // LOGINMODULE_H
