#pragma once
#include "basepage.h"
#include <QJsonObject>
class QLabel; class QPushButton; class QLineEdit; class QDoubleSpinBox; class QComboBox; class QListWidget;
class QStackedWidget; class QScrollArea; class QButtonGroup; class QRadioButton; class QTextEdit; class QGridLayout;

// 量表题目结构
struct Question {
    QString text;
    QStringList options;
    QList<int> scores;
};

struct Scale {
    QString code;
    QString name;
    QString description;
    QList<Question> questions;
    QString interpretation;
};

// EvaluatePage: 健康评估 + 钱包充值 UI
class EvaluatePage : public BasePage {
    Q_OBJECT
public:
    explicit EvaluatePage(CommunicationClient *c, const QString &patient, QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj);
signals:
    void requestBackHome();
private slots:
    void onScaleSelected();
    void doRecharge();
    void goBack();
    void submitScale();
    void resetScale();
private:
    void requestConfig();
    void sendJson(const QJsonObject &o);
    void initializeScales();
    void createScaleWidget(const Scale &scale);
    void calculateAndShowResult();
    
    QLabel *m_balanceLabel; QDoubleSpinBox *m_amountSpin; QLabel *m_statusLabel; QPushButton *m_rechargeBtn; QPushButton *m_backBtn; 
    QListWidget *m_scaleList; QStackedWidget *m_scaleStack; QPushButton *m_submitBtn; QPushButton *m_resetBtn;
    QTextEdit *m_resultText;
    
    QList<Scale> m_scales;
    QList<QList<QButtonGroup*>> m_scaleButtonGroups; // 每个量表有多个问题，每个问题一个按钮组
    int m_currentScaleIndex = -1;
};
