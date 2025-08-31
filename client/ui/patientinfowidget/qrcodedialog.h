#pragma once
#include <QDialog>
#include <QTimer>

class QLabel;
class QPushButton;
class QProgressBar;

// 虚拟二维码充值对话框
class QRCodeDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit QRCodeDialog(double amount, QWidget *parent = nullptr);
    
signals:
    void paymentCompleted(double amount);
    void paymentCancelled();
    
private slots:
    void onTimerTimeout();
    void onCancelClicked();
    void onPayNowClicked();
    
private:
    void setupUI();
    void startPaymentTimer();
    
    double m_amount;
    QLabel *m_qrCodeLabel;
    QLabel *m_amountLabel;
    QLabel *m_instructionLabel;
    QLabel *m_timerLabel;
    QPushButton *m_cancelBtn;
    QPushButton *m_payNowBtn;
    QProgressBar *m_progressBar;
    QTimer *m_timer;
    int m_timeLeft; // 倒计时秒数
};
