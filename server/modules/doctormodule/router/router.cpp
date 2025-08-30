#include "router.h"
#include "core/network/messagerouter.h"
#include "core/network/protocol.h"
#include "modules/doctormodule/profile/profile.h"
#include "modules/doctormodule/assignment/assignment.h"
#include "modules/doctormodule/attendance/attendance.h"
#include "core/logging/logging.h"

DoctorRouterModule::DoctorRouterModule(QObject *parent):QObject(parent) {
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &DoctorRouterModule::onRequest)) {
        Log::error("DoctorRouterModule", "Failed to connect MessageRouter::requestReceived to DoctorRouterModule::onRequest");
    }
    if (!connect(this, &DoctorRouterModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("DoctorRouterModule", "Failed to connect DoctorRouterModule::businessResponse to MessageRouter::onBusinessResponse");
    }
}

void DoctorRouterModule::onRequest(const QJsonObject &payload) {
    const QString a = payload.value("action").toString();
    Log::request("DoctorRouter", payload, "username", payload.value("username").toString());
    QJsonObject resp;
    if (a == "get_doctor_info" || a == "update_doctor_info") {
        DoctorProfileModule m; resp = m.handle(payload);
    } else if (a == "get_doctor_assignment" || a == "update_doctor_assignment") {
        DoctorAssignmentModule m; resp = m.handle(payload);
    } else if (a == "doctor_checkin" || a == "doctor_leave" || a == "get_active_leaves" || a == "cancel_leave" || a == "get_attendance_history") {
        DoctorAttendanceModule m; resp = m.handle(payload);
    } else {
        return; // 非本模块处理
    }
    // 统一在 reply 时打印 response
    reply(resp, payload);
}

void DoctorRouterModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("DoctorRouter", resp);
    emit businessResponse(Protocol::MessageType::JsonResponse, resp);
}
