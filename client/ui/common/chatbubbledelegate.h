#pragma once
#include <QStyledItemDelegate>

namespace ChatItemRoles {
    enum {
        RoleSender = Qt::UserRole + 1,
        RoleText,
        RoleIsOutgoing,
        RoleMessageId
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
