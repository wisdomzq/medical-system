#include "ui/common/chatbubbledelegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>

ChatBubbleDelegate::ChatBubbleDelegate(const QString& currentUser, QObject* parent)
    : QStyledItemDelegate(parent), m_currentUser(currentUser) {}

QSize ChatBubbleDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const QString text = index.data(ChatItemRoles::RoleText).toString();
    QFontMetrics fm(option.font);
    const int maxWidth = option.rect.width() * 0.6; // 气泡最大宽度
    QRect br = fm.boundingRect(0, 0, maxWidth, 0, Qt::TextWordWrap, text);
    const int pad = 12;
    return QSize(option.rect.width(), br.height() + pad * 2 + 6);
}

void ChatBubbleDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const {
    p->save();
    const QString sender = idx.data(ChatItemRoles::RoleSender).toString();
    const QString text = idx.data(ChatItemRoles::RoleText).toString();
    const bool outgoing = idx.data(ChatItemRoles::RoleIsOutgoing).toBool();

    QRect r = opt.rect.adjusted(8, 4, -8, -4);
    const int maxBubbleW = int(r.width() * 0.6);
    QFontMetrics fm(opt.font);
    QRect textRect = fm.boundingRect(0, 0, maxBubbleW, 0, Qt::TextWordWrap, text);
    const int pad = 10;
    QSize bubbleSize(textRect.width() + pad * 2, textRect.height() + pad * 2);

    QRect bubbleRect;
    if (outgoing) {
        bubbleRect = QRect(r.right() - bubbleSize.width(), r.top(), bubbleSize.width(), bubbleSize.height());
    } else {
        bubbleRect = QRect(r.left(), r.top(), bubbleSize.width(), bubbleSize.height());
    }

    // 背景
    QPainterPath path;
    path.addRoundedRect(bubbleRect, 10, 10);
    QColor bg = outgoing ? QColor(0, 122, 255) : QColor(230, 230, 230);
    QColor fg = outgoing ? Qt::white : Qt::black;
    p->fillPath(path, bg);

    // 文本
    p->setPen(fg);
    QRect inner = bubbleRect.adjusted(pad, pad, -pad, -pad);
    QTextOption to; to.setWrapMode(QTextOption::WordWrap);
    p->drawText(inner, text, to);

    // 可选：显示发送者
    p->setPen(QColor(120,120,120));
    const QString label = outgoing ? QString("我 (%1)").arg(sender) : sender;
    p->drawText(QRect(bubbleRect.left(), bubbleRect.bottom()+2, bubbleRect.width(), 14), Qt::AlignLeft|Qt::AlignVCenter, label);

    p->restore();
}
