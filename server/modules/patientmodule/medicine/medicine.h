 #pragma once
 #include <QObject>
 #include <QJsonObject>
 #include <QNetworkAccessManager>
 #include <QNetworkReply>
 #include <QJsonArray>
  class MedicineModule : public QObject {
	 Q_OBJECT
 public:
	 explicit MedicineModule(QObject *parent=nullptr);
 signals:
	 void businessResponse(QJsonObject payload);
 private slots:
	 void onRequest(const QJsonObject &payload);
	 void onRemoteFinished();
 private:
	QNetworkAccessManager m_nam;
	QJsonArray m_localMeta; // 本地元数据缓存
	void loadLocalMeta();
	 void handleGetMedications(const QJsonObject &payload);
	 void handleSearchMedications(const QJsonObject &payload);
	 void handleRemoteSearch(const QJsonObject &payload);
	 void sendResponse(QJsonObject resp, const QJsonObject &orig);
 };
