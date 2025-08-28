#include "tcpserver.h"
#include "database.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

TcpServer::TcpServer(QObject *parent) : QTcpServer(parent), m_dbManager("user.db")
{
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &TcpServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
}

void TcpServer::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket)
    {
        processRequest(socket, socket->readAll());
    }
}

void TcpServer::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket)
    {
        socket->deleteLater();
    }
}

void TcpServer::processRequest(QTcpSocket *socket, const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject request = doc.object();
    QString type = request["type"].toString();
    QJsonObject response;

    if (type == "login")
    {
        QString username = request["username"].toString();
        QString password = request["password"].toString();
        response["type"] = "login_response";
        response["success"] = m_dbManager.authenticateUser(username, password);
    }
    else if (type == "register")
    {
        QString username = request["username"].toString();
        QString password = request["password"].toString();
        QString role = request["role"].toString();
        response["type"] = "register_response";
        response["success"] = m_dbManager.addUser(username, password, role);
    }
    else if (type == "get_doctor_info")
    {
        QString username = request["username"].toString();
        QJsonObject doctorInfo;
        if (m_dbManager.getDoctorInfo(username, doctorInfo))
        {
            response["type"] = "doctor_info_response";
            response["success"] = true;
            response["data"] = doctorInfo;
        }
        else
        {
            response["type"] = "doctor_info_response";
            response["success"] = false;
        }
    }
    else if (type == "get_patient_info")
    {
        QString username = request["username"].toString();
        QJsonObject patientInfo;
        if (m_dbManager.getPatientInfo(username, patientInfo))
        {
            response["type"] = "patient_info_response";
            response["success"] = true;
            response["data"] = patientInfo;
        }
        else
        {
            response["type"] = "patient_info_response";
            response["success"] = false;
        }
    }
    else if (type == "update_doctor_info")
    {
        QString username = request["username"].toString();
        QJsonObject data = request["data"].toObject();
        response["type"] = "update_doctor_info_response";
        response["success"] = m_dbManager.updateDoctorInfo(username, data);
    }
    else if (type == "update_patient_info")
    {
        QString username = request["username"].toString();
        QJsonObject data = request["data"].toObject();
        response["type"] = "update_patient_info_response";
        response["success"] = m_dbManager.updatePatientInfo(username, data);
    }

    socket->write(QJsonDocument(response).toJson());
}
