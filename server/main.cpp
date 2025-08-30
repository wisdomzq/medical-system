#include <QtGlobal>
#include <QCoreApplication>
#include <QDebug>
#include "core/network/src/protocol.h"
#include "core/network/src/server/communicationserver.h"
#include "core/network/src/server/messagerouter.h"
// 已有的模块（自身在构造时会连接 MessageRouter::requestReceived）
#include "modules/patientmodule/medicine/medicine.h"
#include "modules/patientmodule/evaluate/evaluate.h"
#include "modules/patientmodule/prescription/prescription.h"
#include "modules/patientmodule/advice/advice.h"
#include "modules/patientmodule/doctorinfo/doctorinfo.h"
#include "modules/patientmodule/register/register.h"
#include "modules/patientmodule/medicalrecord/medicalrecord.h"
// 列表/目录模块
#include "modules/patientmodule/doctorlist/doctorlist.h"
// 新增的路由/业务模块
#include "modules/loginmodule/loginrouter.h"
#include "modules/patientmodule/patientinfo/patientinfo.h"
#include "modules/patientmodule/appointment/appointment.h"
#include "modules/patientmodule/hospitalization/hospitalization.h"
#include "modules/medicalcrud/medicalcrud.h"
#include "modules/doctormodule/router/router.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    qRegisterMetaType<Protocol::Header>("Protocol::Header");

    CommunicationServer server;
    // 实例化并注册所有业务模块：它们会在构造时各自连接 MessageRouter
    MedicineModule medicineModule;
    EvaluateModule evaluateModule;
    PrescriptionModule prescriptionModule;
    AdviceModule adviceModule;
    DoctorInfoModule doctorInfoModule;
    RegisterManager registerManager;
    MedicalRecordModule medicalRecordModule;
    DoctorListModule doctorListModule;
    // 新增的模块（将 main.cpp 中的 if-else 逻辑下沉到各自模块）
    LoginRouter loginRouter;
    PatientInfoModule patientInfoModule;
    AppointmentModule appointmentModule;
    HospitalizationModule hospitalizationModule;
    MedicalCrudModule medicalCrudModule;
    DoctorRouterModule doctorRouterModule;

    if (server.listen(QHostAddress::Any, Protocol::SERVER_PORT)) {
        qDebug() << "Server started on port" << Protocol::SERVER_PORT;
    } else {
        qWarning() << "Server failed to start on port" << Protocol::SERVER_PORT;
    }
    return a.exec();
}