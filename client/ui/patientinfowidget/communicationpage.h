#pragma once
#include "basepage.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QQueue>

class QListWidget; class QComboBox; class QLineEdit; class QPushButton; class ChatService; class DoctorListService;

class CommunicationPage : public BasePage {
    Q_OBJECT
public:
    CommunicationPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr);
private slots:
    void sendClicked();
    void sendImageClicked();
    void onPeerChanged();
    void loadMore();
    void onHistory(const QJsonArray& msgs, bool hasMore);
    void onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool hasMore);
private:
    void appendMessage(const QJsonObject& m);
    void insertOlderAtTop(const QJsonArray& msgsDesc);
    void populateDoctors();
    QString currentDoctor() const;

    QListWidget* m_list {nullptr};
    QListWidget* m_doctorList {nullptr};
    QLineEdit* m_input {nullptr};
    QPushButton* m_sendBtn {nullptr};
    QPushButton* m_sendImageBtn {nullptr};
    QPushButton* m_loadMoreBtn {nullptr};
    ChatService* m_chat {nullptr};
    DoctorListService* m_doctorService {nullptr};
    qint64 m_cursor {0};
    QMap<QString, qint64> m_earliestByPeer;
    QString m_currentPeer;
    // 图片下载队列
    QQueue<QPair<QString, QString>> m_downloadQueue;
    bool m_downloading {false};

    void tryStartNextDownload();
};
