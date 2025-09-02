#ifndef CHATROOMWIDGET_H
#define CHATROOMWIDGET_H

#include <QWidget>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QQueue>
class QListWidget;
class QLineEdit;
class QPushButton;
class CommunicationClient;
class ChatService;

class ChatRoomWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatRoomWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent=nullptr);
private slots:
    void sendClicked();
    void sendImageClicked(); // Added declaration for sendImageClicked slot
    void addPeer();
    void onPeerChanged();
    void loadMore();
    void onHistory(const QJsonArray& msgs, bool hasMore);
    void onEvents(const QJsonArray& msgs, const QJsonArray& instant, qint64 nextCursor, bool hasMore);
private:
    void appendMessage(const QJsonObject& m);
    void insertOlderAtTop(const QJsonArray& msgsDesc);
    void tryStartNextDownload();
    QString m_doctor;
    QString m_currentPeer; // 患者用户名（简化：可编辑输入）
    CommunicationClient* m_client {nullptr};
    ChatService* m_chat {nullptr};
    QListWidget* m_convList {nullptr};
    QListWidget* m_list {nullptr};
    QLineEdit* m_peerEdit {nullptr};
    QPushButton* m_addPeerBtn {nullptr};
    QLineEdit* m_input {nullptr};
    QPushButton* m_sendBtn {nullptr};
    QPushButton* m_sendImageBtn {nullptr};
    QPushButton* m_loadMoreBtn {nullptr};
    qint64 m_cursor {0};
    QMap<QString, qint64> m_earliestByPeer; // 每个会话的最早消息id（用于翻页）
    // 图片下载队列（serverName -> localPath）
    QQueue<QPair<QString, QString>> m_downloadQueue;
    bool m_downloading {false};
};

#endif // CHATROOMWIDGET_H
