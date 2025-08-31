#pragma once
#include <QDialog>

class QLabel;
class QPushButton;
class QPropertyAnimation;

// 充值成功对话框
class PaymentSuccessDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit PaymentSuccessDialog(double amount, double newBalance, QWidget *parent = nullptr);
    
private slots:
    void onConfirmClicked();
    
private:
    void setupUI();
    void startSuccessAnimation();
    
    double m_amount;
    double m_newBalance;
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_amountLabel;
    QLabel *m_balanceLabel;
    QPushButton *m_confirmBtn;
    QPropertyAnimation *m_animation;
};
