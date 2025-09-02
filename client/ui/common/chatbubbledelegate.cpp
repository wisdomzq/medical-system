#include "ui/common/chatbubbledelegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>

ChatBubbleDelegate::ChatBubbleDelegate(const QString& currentUser, QObject* parent)
    : QStyledItemDelegate(parent), m_currentUser(currentUser) {}

QSize ChatBubbleDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const QString type = index.data(ChatItemRoles::RoleMessageType).toString();
    const int maxWidth = option.rect.width() * 0.6;
    const int pad = 12;
    if (type == "image") {
        // 预估图片显示尺寸，若本地已缓存则按缩略图大小
        const QString local = index.data(ChatItemRoles::RoleImageLocalPath).toString();
        QSize imgSz(160, 120);
        if (!local.isEmpty()) {
            QImage img(local);
            if (!img.isNull()) {
                imgSz = img.size().scaled(QSize(maxWidth, 300), Qt::KeepAspectRatio);
            }
        }
        return QSize(option.rect.width(), imgSz.height() + pad * 2 + 20);
    } else {
        const QString text = index.data(ChatItemRoles::RoleText).toString();
        QFontMetrics fm(option.font);
        QRect br = fm.boundingRect(0, 0, maxWidth, 0, Qt::TextWordWrap, text);
        return QSize(option.rect.width(), br.height() + pad * 2 + 6);
    }
}

void ChatBubbleDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const {
    p->save();
    const QString sender = idx.data(ChatItemRoles::RoleSender).toString();
    const bool outgoing = idx.data(ChatItemRoles::RoleIsOutgoing).toBool();
    const QString type = idx.data(ChatItemRoles::RoleMessageType).toString();

    QRect r = opt.rect.adjusted(8, 4, -8, -4);
    const int maxBubbleW = int(r.width() * 0.6);
    QFontMetrics fm(opt.font);
    const int pad = 10;
    QSize bubbleSize;
    QRect contentRect;
    if (type == "image") {
        const QString local = idx.data(ChatItemRoles::RoleImageLocalPath).toString();
        QImage img(local);
        QSize imgSz = img.isNull() ? QSize(160, 120) : img.size().scaled(QSize(maxBubbleW, 300), Qt::KeepAspectRatio);
        bubbleSize = QSize(imgSz.width() + pad * 2, imgSz.height() + pad * 2);
        contentRect = QRect(QPoint(0,0), imgSz);
    } else {
        const QString text = idx.data(ChatItemRoles::RoleText).toString();
        QRect textRect = fm.boundingRect(0, 0, maxBubbleW, 0, Qt::TextWordWrap, text);
        bubbleSize = QSize(textRect.width() + pad * 2, textRect.height() + pad * 2);
        contentRect = textRect;
    }

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

    QRect inner = bubbleRect.adjusted(pad, pad, -pad, -pad);
    if (type == "image") {
        const QString local = idx.data(ChatItemRoles::RoleImageLocalPath).toString();
        QImage img(local);
        if (!img.isNull()) {
            QImage scaled = img.scaled(inner.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPoint topLeft = inner.topLeft();
            topLeft.setX(topLeft.x() + (inner.width() - scaled.width())/2);
            topLeft.setY(topLeft.y() + (inner.height() - scaled.height())/2);
            p->drawImage(QRect(topLeft, scaled.size()), scaled);
        } else {
            // 占位符
            p->setPen(fg);
            p->drawText(inner, Qt::AlignCenter, QObject::tr("[图片]"));
        }
    } else {
        // 文本
        const QString text = idx.data(ChatItemRoles::RoleText).toString();
        p->setPen(fg);
        QTextOption to; to.setWrapMode(QTextOption::WordWrap);
        p->drawText(inner, text, to);
    }

    // 可选：显示发送者
    p->setPen(QColor(120,120,120));
    const QString label = outgoing ? QString("我 (%1)").arg(sender) : sender;
    p->drawText(QRect(bubbleRect.left(), bubbleRect.bottom()+2, bubbleRect.width(), 14), Qt::AlignLeft|Qt::AlignVCenter, label);

    p->restore();
}
