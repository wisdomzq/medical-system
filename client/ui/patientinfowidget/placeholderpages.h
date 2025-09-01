#pragma once
#include "basepage.h"
#include <QJsonArray>
#include <QJsonObject>
class QListWidget; class QLineEdit; class QPushButton; class ChatService;

class CommunicationPage : public BasePage {
	Q_OBJECT
public:
	CommunicationPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr);
private slots:
	void sendClicked();
	void addPeer();
	void onPeerChanged();
	void loadMore();
	void onHistory(const QJsonArray& msgs, bool hasMore);
	void onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool hasMore);
private:
	void appendMessage(const QJsonObject& m);
	void schedulePoll();
	void insertOlderAtTop(const QJsonArray& msgsDesc);
	QListWidget* m_list {nullptr};
	QListWidget* m_convList {nullptr};
	QLineEdit* m_peerEdit {nullptr};
	QLineEdit* m_input {nullptr};
	QPushButton* m_sendBtn {nullptr};
	QPushButton* m_addPeerBtn {nullptr};
	QPushButton* m_loadMoreBtn {nullptr};
	ChatService* m_chat {nullptr};
	qint64 m_cursor {0};
	QMap<QString, qint64> m_earliestByPeer;
};

// 保留健康评估占位；医生信息已在 doctorinfopage.h 定义，避免重名冲突
class HealthAssessmentPage : public BasePage { public: HealthAssessmentPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
