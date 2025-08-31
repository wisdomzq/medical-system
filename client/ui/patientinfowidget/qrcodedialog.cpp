#include "qrcodedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QPixmap>
#include <QTimer>
#include <QFont>
#include <QApplication>
#include <QStyleOption>
#include <QPainter>

QRCodeDialog::QRCodeDialog(double amount, QWidget *parent)
    : QDialog(parent), m_amount(amount), m_timeLeft(60) {
    setWindowTitle(tr("扫码充值"));
    setModal(true);
    setFixedSize(400, 500);
    setupUI();
    startPaymentTimer();
}

void QRCodeDialog::setupUI() {
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setSpacing(15);
    mainLay->setContentsMargins(20, 20, 20, 20);
    
    // 标题
    auto *titleLabel = new QLabel(tr("请使用手机支付宝/微信扫码付款"));
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLay->addWidget(titleLabel);
    
    // 金额显示
    m_amountLabel = new QLabel(tr("充值金额: ¥%1").arg(QString::number(m_amount, 'f', 2)));
    m_amountLabel->setAlignment(Qt::AlignCenter);
    QFont amountFont = m_amountLabel->font();
    amountFont.setPointSize(16);
    amountFont.setBold(true);
    m_amountLabel->setFont(amountFont);
    m_amountLabel->setStyleSheet("color: #ff6600; padding: 10px;");
    mainLay->addWidget(m_amountLabel);
    
    // 二维码显示区域
    m_qrCodeLabel = new QLabel();
    m_qrCodeLabel->setFixedSize(200, 200);
    m_qrCodeLabel->setAlignment(Qt::AlignCenter);
    m_qrCodeLabel->setStyleSheet("border: 2px solid #cccccc; background-color: #f9f9f9;");
    
    // 创建虚拟二维码图片
    QPixmap qrPixmap(180, 180);
    qrPixmap.fill(Qt::white);
    QPainter painter(&qrPixmap);
    painter.setPen(QPen(Qt::black, 2));
    
    // 画一个简单的二维码样式（方块图案）
    int blockSize = 10;
    bool pattern[18][18] = {
        {1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,1,0,1,0,1,1,0,0,0,0,0,1},
        {1,0,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1},
        {1,0,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1},
        {1,0,1,1,1,0,1,0,0,0,0,1,0,1,1,1,0,1},
        {1,0,0,0,0,0,1,0,1,0,1,1,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
        {0,1,0,1,1,0,1,1,0,0,1,1,0,1,0,1,1,0},
        {1,0,1,0,0,1,0,0,1,1,0,0,1,0,1,0,0,1},
        {0,1,1,1,0,0,1,1,0,1,1,1,0,1,1,1,0,0},
        {1,0,0,0,1,1,0,0,1,0,0,0,1,0,0,0,1,1},
        {0,1,1,1,0,0,1,1,0,1,1,1,0,1,1,1,0,0},
        {0,0,0,0,0,0,0,0,1,0,1,0,0,1,0,1,1,0},
        {1,1,1,1,1,1,1,0,0,1,0,1,1,0,1,0,0,1},
        {1,0,0,0,0,0,1,0,1,0,1,0,0,1,1,1,0,0},
        {1,0,1,1,1,0,1,0,0,1,0,1,1,0,1,0,1,1},
        {1,1,1,1,1,1,1,0,1,0,1,0,0,1,0,1,0,0}
    };
    
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 18; j++) {
            if (pattern[i][j]) {
                painter.fillRect(j * blockSize, i * blockSize, blockSize, blockSize, Qt::black);
            }
        }
    }
    
    m_qrCodeLabel->setPixmap(qrPixmap);
    
    auto *qrLay = new QHBoxLayout();
    qrLay->addStretch();
    qrLay->addWidget(m_qrCodeLabel);
    qrLay->addStretch();
    mainLay->addLayout(qrLay);
    
    // 说明文字
    m_instructionLabel = new QLabel(tr("请在1分钟内完成支付\n支付完成后，系统将自动确认到账"));
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setStyleSheet("color: #666666; line-height: 1.5;");
    mainLay->addWidget(m_instructionLabel);
    
    // 倒计时显示
    m_timerLabel = new QLabel();
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet("color: #ff0000; font-weight: bold;");
    mainLay->addWidget(m_timerLabel);
    
    // 进度条
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 60);
    m_progressBar->setValue(60);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid grey;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #05B8CC;"
        "    border-radius: 3px;"
        "}"
    );
    mainLay->addWidget(m_progressBar);
    
    mainLay->addStretch();
    
    // 按钮区域
    auto *btnLay = new QHBoxLayout();
    m_cancelBtn = new QPushButton(tr("取消"));
    m_payNowBtn = new QPushButton(tr("我已完成支付"));
    
    m_cancelBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #cccccc;"
        "    border: none;"
        "    padding: 8px 20px;"
        "    border-radius: 4px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #bbbbbb;"
        "}"
    );
    
    m_payNowBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #07c160;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 20px;"
        "    border-radius: 4px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #06ad56;"
        "}"
    );
    
    connect(m_cancelBtn, &QPushButton::clicked, this, &QRCodeDialog::onCancelClicked);
    connect(m_payNowBtn, &QPushButton::clicked, this, &QRCodeDialog::onPayNowClicked);
    
    btnLay->addWidget(m_cancelBtn);
    btnLay->addStretch();
    btnLay->addWidget(m_payNowBtn);
    mainLay->addLayout(btnLay);
}

void QRCodeDialog::startPaymentTimer() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &QRCodeDialog::onTimerTimeout);
    m_timer->start(1000); // 每秒更新一次
    onTimerTimeout(); // 立即更新显示
}

void QRCodeDialog::onTimerTimeout() {
    m_timerLabel->setText(tr("剩余时间: %1 秒").arg(m_timeLeft));
    m_progressBar->setValue(m_timeLeft);
    
    if (m_timeLeft <= 0) {
        m_timer->stop();
        m_timerLabel->setText(tr("支付超时"));
        m_payNowBtn->setEnabled(false);
        m_cancelBtn->setText(tr("关闭"));
        return;
    }
    
    m_timeLeft--;
}

void QRCodeDialog::onCancelClicked() {
    if (m_timer) {
        m_timer->stop();
    }
    emit paymentCancelled();
    reject();
}

void QRCodeDialog::onPayNowClicked() {
    if (m_timer) {
        m_timer->stop();
    }
    emit paymentCompleted(m_amount);
    accept();
}
