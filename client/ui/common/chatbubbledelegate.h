#pragma once
#include <QStyledItemDelegate>

namespace ChatItemRoles {
    enum {
        RoleSender = Qt::UserRole + 1,
        RoleText,
    RoleIsOutgoing,
    RoleMessageId,
    RoleMessageType,       // "text" | "image" | others
    RoleImageServerName,   // 服务器保存名（用于比对下载完成）
    RoleImageLocalPath     // 本地缓存路径（存在时用于渲染）
    };
}

class ChatBubbleDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ChatBubbleDelegate(const QString& currentUser, QObject* parent=nullptr);
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    QString m_currentUser;
};
