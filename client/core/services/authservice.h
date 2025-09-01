#pragma once

#include <QObject>
#include <QJsonObject>

class CommunicationClient;

// 认证服务：负责登录/注册等业务逻辑与网络交互
class AuthService : public QObject {
    Q_OBJECT
public:
    explicit AuthService(QObject* parent = nullptr);
    // 使用共享的 CommunicationClient，不拥有该指针
    explicit AuthService(CommunicationClient* sharedClient, QObject* parent = nullptr);
    ~AuthService();

    // 连接到默认服务器（可扩展为可配置）
    void connectDefault();

    // 登录/注册 API（UI 调用这些方法，服务内部封装请求构造与发送、响应处理）
    void login(const QString& username, const QString& password);
    void registerDoctor(const QString& username, const QString& password,
                        const QString& department, const QString& phone);
    void registerPatient(const QString& username, const QString& password,
                         int age, const QString& phone, const QString& address);

signals:
    // 连接状态（UI 可选择显示提示）
    void connected();
    void disconnected();
    void networkError(int code, const QString& message);

    // 登录/注册结果
    void loginSucceeded(const QString& role, const QString& username, const QString& message);
    void loginFailed(const QString& message);
    void registerSucceeded(const QString& role, const QString& message);
    void registerFailed(const QString& message);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr;
    QString m_lastLoginUsername;
};
