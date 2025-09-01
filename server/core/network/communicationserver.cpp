#include "core/network/clienthandler.h"
#include "core/network/communicationserver.h"
#include "core/network/messagerouter.h"

#include <QThread>
#include "core/logging/logging.h"

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

    if (!connect(handler, &QObject::destroyed, thread, &QThread::quit)) {
        Log::error("CommunicationServer", "Failed to connect ClientHandler::destroyed to QThread::quit");
    }
    if (!connect(thread, &QThread::finished, thread, &QThread::deleteLater)) {
        Log::error("CommunicationServer", "Failed to connect QThread::finished to QThread::deleteLater");
    }
    // 当线程启动后，通过信号和槽机制调用 handler 的初始化方法
    if (!connect(thread, &QThread::started, handler, [handler, socketDescriptor]() {
        qInfo() << "[Server] 在线程" << QThread::currentThread() << "中初始化 ClientHandler";
        handler->initialize(socketDescriptor);
    })) {
        Log::error("CommunicationServer", "Failed to connect QThread::started to handler lambda");
    }

    if (!QObject::connect(handler, &ClientHandler::requestJsonReady,
        &MessageRouter::instance(), &MessageRouter::onJsonRequest,
        Qt::QueuedConnection)) {
        Log::error("CommunicationServer", "Failed to connect ClientHandler::requestJsonReady to MessageRouter::onJsonRequest");
    }
    // handler 销毁时让路由器清理路由
    if (!QObject::connect(handler, &QObject::destroyed, &MessageRouter::instance(), &MessageRouter::onClientHandlerDestroyed,
                          Qt::QueuedConnection)) {
        Log::error("CommunicationServer", "Failed to connect ClientHandler::destroyed to MessageRouter::onClientHandlerDestroyed");
    }
    
    // 将调度器的响应信号转发到对应的 handler::sendMessage（仅当目标是该 handler 时）
    if (!QObject::connect(&MessageRouter::instance(), &MessageRouter::responseReady, handler, [handler](ClientHandler* target, QJsonObject payload) {
                         if (target != handler) return; // 只处理发给该 handler 的响应
                         handler->onJsonResponseReady(payload);
                     }, Qt::QueuedConnection)) {
        Log::error("CommunicationServer", "Failed to connect MessageRouter::responseReady to handler lambda");
    }

    qInfo() << "[Server] 启动处理线程" << thread;
    thread->start();
}
