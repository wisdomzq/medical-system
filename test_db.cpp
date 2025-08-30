#include "server/core/database/database.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    DBManager db("/home/zephyr/medical-system/data/user.db");
    
    QJsonArray appointments;
    bool result = db.getAppointmentsByDoctor("委员长", appointments);
    
    qDebug() << "查询结果:" << result;
    qDebug() << "预约数量:" << appointments.size();
    
    for (int i = 0; i < appointments.size(); ++i) {
        qDebug() << "预约" << i+1 << ":" << appointments[i].toObject();
    }
    
    return 0;
}
