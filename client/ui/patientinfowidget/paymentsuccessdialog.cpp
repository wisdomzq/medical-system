#include "paymentsuccessdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QPixmap>
#include <QPainter>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>

PaymentSuccessDialog::PaymentSuccessDialog(double amount, double newBalance, QWidget *parent)
    : QDialog(parent), m_amount(amount), m_newBalance(newBalance) {
    setWindowTitle(tr("充值成功"));
    setModal(true);
    setFixedSize(350, 400);
    setupUI();
    startSuccessAnimation();
}

void PaymentSuccessDialog::setupUI() {
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setSpacing(20);
    mainLay->setContentsMargins(30, 30, 30, 30);
    
    mainLay->addStretch();
    
    // 成功图标
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(80, 80);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    
    // 创建一个绿色对勾图标
    QPixmap iconPixmap(80, 80);
    iconPixmap.fill(Qt::transparent);
    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆形背景
    painter.setBrush(QBrush(QColor(7, 193, 96))); // 微信绿色
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 80, 80);
    
    // 绘制对勾
    painter.setPen(QPen(Qt::white, 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(20, 40, 35, 55);
    painter.drawLine(35, 55, 60, 25);
    
    m_iconLabel->setPixmap(iconPixmap);
    
    auto *iconLay = new QHBoxLayout();
    iconLay->addStretch();
    iconLay->addWidget(m_iconLabel);
    iconLay->addStretch();
    mainLay->addLayout(iconLay);
    
    // 成功标题
    m_titleLabel = new QLabel(tr("充值成功！"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #07c160; margin: 10px 0;");
    mainLay->addWidget(m_titleLabel);
    
    // 充值金额
    m_amountLabel = new QLabel(tr("充值金额: ¥%1").arg(QString::number(m_amount, 'f', 2)));
    m_amountLabel->setAlignment(Qt::AlignCenter);
    QFont amountFont = m_amountLabel->font();
    amountFont.setPointSize(16);
    m_amountLabel->setFont(amountFont);
    m_amountLabel->setStyleSheet("color: #333333; margin: 5px 0;");
    mainLay->addWidget(m_amountLabel);
    
    // 当前余额
    m_balanceLabel = new QLabel(tr("当前余额: ¥%1").arg(QString::number(m_newBalance, 'f', 2)));
    m_balanceLabel->setAlignment(Qt::AlignCenter);
    QFont balanceFont = m_balanceLabel->font();
    balanceFont.setPointSize(14);
    m_balanceLabel->setFont(balanceFont);
    m_balanceLabel->setStyleSheet("color: #666666; margin: 5px 0;");
    mainLay->addWidget(m_balanceLabel);
    
    mainLay->addStretch();
    
    // 确认按钮
    m_confirmBtn = new QPushButton(tr("确定"));
    m_confirmBtn->setFixedHeight(40);
    m_confirmBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #07c160;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 30px;"
        "    border-radius: 6px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #06ad56;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #059a4c;"
        "}"
    );
    
    connect(m_confirmBtn, &QPushButton::clicked, this, &PaymentSuccessDialog::onConfirmClicked);
    
    auto *btnLay = new QHBoxLayout();
    btnLay->addStretch();
    btnLay->addWidget(m_confirmBtn);
    btnLay->addStretch();
    mainLay->addLayout(btnLay);
    
    mainLay->addStretch();
}

void PaymentSuccessDialog::startSuccessAnimation() {
    // 为图标添加透明度效果
    auto *opacityEffect = new QGraphicsOpacityEffect(this);
    m_iconLabel->setGraphicsEffect(opacityEffect);
    
    // 创建淡入动画
    m_animation = new QPropertyAnimation(opacityEffect, "opacity", this);
    m_animation->setDuration(800);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    
    // 延迟一点开始动画，让对话框先显示
    QTimer::singleShot(100, [this]() {
        m_animation->start();
    });
}

void PaymentSuccessDialog::onConfirmClicked() {
    accept();
}
