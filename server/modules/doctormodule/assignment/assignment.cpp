#include "assignment.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "core/database/database.h"
#include "core/database/database_config.h"

QJsonObject DoctorAssignmentModule::handle(const QJsonObject& payload) {
	const QString action = payload.value("action").toString();
	if (action == "get_doctor_assignment") return handleGet(payload);
	if (action == "update_doctor_assignment") return handleUpdate(payload);
	QJsonObject resp; resp["type"] = "assignment_unknown_response"; resp["success"] = false; resp["error"] = QStringLiteral("Unknown action: %1").arg(action); return resp;
}

QJsonObject DoctorAssignmentModule::handleGet(const QJsonObject& payload) {
	const QString username = payload.value("username").toString(payload.value("doctor_username").toString());
	DBManager db(DatabaseConfig::getDatabasePath());
	QJsonObject info; bool ok = db.getDoctorInfo(username, info);
	QJsonObject data;
	data["username"] = username;
	data["work_time"] = info.value("title").toString();
	data["max_patients_per_day"] = info.value("max_patients_per_day").toInt();
	QJsonObject resp; resp["type"] = "get_doctor_assignment_response"; resp["success"] = ok; resp["data"] = data; return resp;
}

QJsonObject DoctorAssignmentModule::handleUpdate(const QJsonObject& payload) {
	const QString username = payload.value("username").toString(payload.value("doctor_username").toString());
	const QJsonObject data = payload.value("data").toObject();
	// 将 work_time 写回 doctors.title；上限写回 max_patients_per_day
	QJsonObject patch;
	if (data.contains("work_time")) patch["title"] = data.value("work_time").toString();
	if (data.contains("max_patients_per_day")) patch["max_patients_per_day"] = data.value("max_patients_per_day");

	DBManager db(DatabaseConfig::getDatabasePath());
	bool ok = db.updateDoctorInfo(username, patch);
	QJsonObject resp; resp["type"] = "update_doctor_assignment_response"; resp["success"] = ok; return resp;
}

