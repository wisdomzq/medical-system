#pragma once
#include <QObject>
#include <QJsonObject>

// PrescriptionModule: 处方模块
// 动作:
// 1. prescription_get_list -> 获取患者的处方列表
// 2. prescription_get_details -> 获取处方详细信息
class PrescriptionModule : public QObject {
    Q_OBJECT
public:
    explicit PrescriptionModule(QObject *parent=nullptr);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleGetList(const QJsonObject &payload);
    void handleGetDetails(const QJsonObject &payload);
    void sendResponse(QJsonObject resp, const QJsonObject &orig);
};
