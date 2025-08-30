#include "attendance.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include <QDate>
#include <QTime>

QJsonObject DoctorAttendanceModule::handle(const QJsonObject &request) {
	const QString action = request.value("action").toString();
	if (action == "doctor_checkin") return handleCheckin(request);
	if (action == "doctor_leave") return handleLeave(request);
	if (action == "get_active_leaves") return handleGetActiveLeaves(request);
	if (action == "cancel_leave") return handleCancelLeave(request);
	if (action == "get_attendance_history") {
		QJsonObject resp; resp["type"] = "attendance_history_response"; resp["success"] = false;
		const QString username = request.value("doctor_username").toString(request.value("username").toString());
		int limit = request.value("limit").toInt(100);
		if (username.isEmpty()) { resp["message"] = "doctor_username required"; return resp; }
		DBManager db(DatabaseConfig::getDatabasePath());
		QJsonArray arr; bool ok = db.getAttendanceByDoctor(username, arr, limit);
		resp["success"] = ok; if (ok) { resp["data"] = arr; } else { resp["message"] = QStringLiteral("query failed"); }
		return resp;
	}
	QJsonObject resp; resp["type"] = "unknown_response"; resp["success"] = false; resp["error"] = QString("Unknown action: %1").arg(action); return resp;
}

QJsonObject DoctorAttendanceModule::handleCheckin(const QJsonObject &request) {
	QJsonObject resp; resp["type"] = "doctor_checkin_response"; resp["success"] = false;
	const QString username = request.value("doctor_username").toString(request.value("username").toString());
	QString date = request.value("checkin_date").toString();
	QString time = request.value("checkin_time").toString();
	if (date.isEmpty()) date = QDate::currentDate().toString("yyyy-MM-dd");
	if (time.isEmpty()) time = QTime::currentTime().toString("HH:mm:ss");
	if (username.isEmpty()) { resp["message"] = "doctor_username required"; return resp; }
	DBManager db(DatabaseConfig::getDatabasePath());
	QJsonObject data {{"doctor_username", username}, {"checkin_date", date}, {"checkin_time", time}};
	bool ok = db.createAttendanceRecord(data);
	resp["success"] = ok;
	if (ok) {
		// 返回最新一条打卡记录（包含 created_at 等）
		QJsonArray arr; if (db.getAttendanceByDoctor(username, arr, 1) && !arr.isEmpty()) {
			resp["data"] = arr.first().toObject();
		} else {
			resp["data"] = data;
		}
	}
	return resp;
}

QJsonObject DoctorAttendanceModule::handleLeave(const QJsonObject &request) {
	QJsonObject resp; resp["type"] = "doctor_leave_response"; resp["success"] = false;
	const QString username = request.value("doctor_username").toString(request.value("username").toString());
	QString leaveDate = request.value("leave_date").toString();
	const QString reason = request.value("reason").toString();
	if (leaveDate.isEmpty()) leaveDate = QDate::currentDate().toString("yyyy-MM-dd");
	if (username.isEmpty()) { resp["message"] = "doctor_username required"; return resp; }
	DBManager db(DatabaseConfig::getDatabasePath());
	QJsonObject data {{"doctor_username", username}, {"leave_date", leaveDate}, {"reason", reason}};
	bool ok = db.createLeaveRequest(data);
	resp["success"] = ok; if (ok) { resp["data"] = data; }
	return resp;
}

QJsonObject DoctorAttendanceModule::handleGetActiveLeaves(const QJsonObject &request) {
	QJsonObject resp; resp["type"] = "active_leaves_response"; resp["success"] = false;
	const QString username = request.value("doctor_username").toString(request.value("username").toString());
	if (username.isEmpty()) { resp["message"] = "doctor_username required"; return resp; }
	DBManager db(DatabaseConfig::getDatabasePath());
	QJsonArray arr; bool ok = db.getActiveLeavesByDoctor(username, arr);
	resp["success"] = ok; if (ok) resp["data"] = arr; return resp;
}

QJsonObject DoctorAttendanceModule::handleCancelLeave(const QJsonObject &request) {
	QJsonObject resp; resp["type"] = "cancel_leave_response"; resp["success"] = false;
	DBManager db(DatabaseConfig::getDatabasePath());
	if (request.contains("leave_id")) {
		bool ok = db.cancelLeaveById(request.value("leave_id").toInt());
		resp["success"] = ok; return resp;
	}
	const QString username = request.value("doctor_username").toString(request.value("username").toString());
	if (username.isEmpty()) { resp["message"] = "doctor_username or leave_id required"; return resp; }
	bool ok = db.cancelActiveLeaveForDoctor(username);
	resp["success"] = ok; return resp;
}
