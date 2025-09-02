#pragma once

#include <QDebug>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QFile>

class StreamFrameParser;
class ResponseDispatcher;

// 客户端通信封装：
// - 负责与服务器建立 TCP 连接
// - 进行认证、心跳维持
// - 发送/接收 JSON 请求与响应
class CommunicationClient : public QObject {
    Q_OBJECT
public:
    explicit CommunicationClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);

signals:
    void connected();
    void disconnected();
    void jsonReceived(const QJsonObject& obj);
    void errorOccurred(int code, const QString& message);
    // 文件下载完成：携带服务器路径名与本地保存路径
    void fileDownloaded(const QString& serverPath, const QString& localPath);

public slots:
    void sendJson(const QJsonObject& obj);
    // 最简文件上传/下载 API（无需鉴权）
    bool uploadFile(const QString& localPath, const QString& serverPath);
    bool downloadFile(const QString& serverPath, const QString& localPath);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void sendHeartbeat();
    void onHeartbeatTimeout();

private:
    // TCP 套接字（直接成员，随对象生命周期自动析构）
    QTcpSocket m_socket;
    // 解析与分发
    StreamFrameParser* m_parser = nullptr;
    ResponseDispatcher* m_dispatcher = nullptr;
    QTimer m_pingTimer;
    QTimer m_pongTimeoutTimer;
    int m_reconnectDelay = 1000; // ms
    QString m_host;
    quint16 m_port = 0;
    // 下载临时文件句柄
    QScopedPointer<QFile> m_downloadFile;
    // 当前下载的标识与目标路径（用于在完成时发信号）
    QString m_currentDownloadServerPath;
    QString m_currentDownloadLocalPath;
};
