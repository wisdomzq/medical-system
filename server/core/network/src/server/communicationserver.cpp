#include <server/clienthandler.h>
#include <server/communicationserver.h>
#include <server/messagerouter.h>

#include <QThread>

CommunicationServer::CommunicationServer(QObject* parent)
    : QTcpServer(parent)
{
}

void CommunicationServer::incomingConnection(qintptr socketDescriptor)
{
    qInfo() << "[Server] 新连接到达，socketDescriptor=" << socketDescriptor;
    QThread* thread = new QThread;
    ClientHandler* handler = new ClientHandler;
    handler->moveToThread(thread);

    connect(handler, &QObject::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    // 当线程启动后，通过信号和槽机制调用 handler 的初始化方法
    connect(thread, &QThread::started, handler, [handler, socketDescriptor]() {
        qInfo() << "[Server] 在线程" << QThread::currentThread() << "中初始化 ClientHandler";
        handler->initialize(socketDescriptor);
    });

    QObject::connect(handler, &ClientHandler::requestReady,
        &MessageRouter::instance(), &MessageRouter::onRequestReady,
        Qt::QueuedConnection);

    // 将调度器的响应信号转发到对应的 handler::sendMessage（仅当目标是该 handler 时）
    QObject::connect(&MessageRouter::instance(), &MessageRouter::responseReady, handler, [handler](ClientHandler* target, Protocol::MessageType type, QJsonObject payload) {
                         if (target != handler) return; // 只处理发给该 handler 的响应
                         handler->sendMessage(type, payload); }, Qt::QueuedConnection);

    qInfo() << "[Server] 启动处理线程" << thread;
    thread->start();
}
