#include "evaluatepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QAbstractItemView>
#include <QJsonArray>
#include <QJsonObject>
#include <QStackedWidget>
#include <QScrollArea>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTextEdit>
#include <QMessageBox>
#include <QGroupBox>
#include "core/network/communicationclient.h"
#include "core/services/evaluateservice.h"
#include "qrcodedialog.h"
#include "paymentsuccessdialog.h"

EvaluatePage::EvaluatePage(CommunicationClient *c, const QString &patient, QWidget *parent)
    : BasePage(c, patient, parent) {
    auto *mainLay = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("<h2>健康评估 & 账户充值</h2>")); 
    mainLay->addWidget(title);

    // 余额显示
    m_balanceLabel = new QLabel(tr("当前余额: 加载中...")); 
    mainLay->addWidget(m_balanceLabel);

    // 充值区域
    auto *rechargeBox = new QGroupBox(tr("账户充值"));
    auto *rechargeLay = new QHBoxLayout(rechargeBox);
    m_amountSpin = new QDoubleSpinBox(); 
    m_amountSpin->setRange(0.01, 10000.0); 
    m_amountSpin->setDecimals(2); 
    m_amountSpin->setValue(10.0);
    m_rechargeBtn = new QPushButton(tr("充值"));
    connect(m_rechargeBtn, &QPushButton::clicked, this, &EvaluatePage::doRecharge);
    rechargeLay->addWidget(new QLabel(tr("金额:"))); 
    rechargeLay->addWidget(m_amountSpin); 
    rechargeLay->addWidget(m_rechargeBtn); 
    rechargeLay->addStretch();
    mainLay->addWidget(rechargeBox);

    // 量表评估区域
    auto *scaleBox = new QGroupBox(tr("健康评估量表"));
    auto *scaleLay = new QHBoxLayout(scaleBox);
    
    // 左侧量表列表
    m_scaleList = new QListWidget();
    m_scaleList->setMaximumWidth(200);
    connect(m_scaleList, &QListWidget::currentRowChanged, this, &EvaluatePage::onScaleSelected);
    scaleLay->addWidget(m_scaleList);
    
    // 右侧量表内容
    auto *rightLay = new QVBoxLayout();
    m_scaleStack = new QStackedWidget();
    rightLay->addWidget(m_scaleStack);
    
    // 控制按钮
    auto *btnLay = new QHBoxLayout();
    m_submitBtn = new QPushButton(tr("提交评估"));
    m_resetBtn = new QPushButton(tr("重新填写"));
    connect(m_submitBtn, &QPushButton::clicked, this, &EvaluatePage::submitScale);
    connect(m_resetBtn, &QPushButton::clicked, this, &EvaluatePage::resetScale);
    btnLay->addWidget(m_submitBtn);
    btnLay->addWidget(m_resetBtn);
    btnLay->addStretch();
    rightLay->addLayout(btnLay);
    
    // 结果显示
    m_resultText = new QTextEdit();
    m_resultText->setMaximumHeight(150);
    m_resultText->setReadOnly(true);
    rightLay->addWidget(m_resultText);
    
    scaleLay->addLayout(rightLay);
    mainLay->addWidget(scaleBox);

    // 状态标签
    m_statusLabel = new QLabel(); 
    mainLay->addWidget(m_statusLabel);

    // 返回按钮
    m_backBtn = new QPushButton(tr("返回首页"));
    connect(m_backBtn, &QPushButton::clicked, this, &EvaluatePage::goBack);
    mainLay->addWidget(m_backBtn);

    // 初始化量表数据
    initializeScales();
    
    // 服务化
    m_service = new EvaluateService(m_client, this);
    connect(m_service, &EvaluateService::configReceived, this, [this](double bal){
        m_balanceLabel->setText(tr("当前余额: %1 元").arg(QString::number(bal,'f',2)));
        m_statusLabel->setText(tr("配置已刷新"));
    });
    connect(m_service, &EvaluateService::configFailed, this, [this](const QString& err){
        m_statusLabel->setText(tr("获取配置失败: %1").arg(err));
    });
    connect(m_service, &EvaluateService::rechargeSucceeded, this, [this](double bal, double amount){
        m_balanceLabel->setText(tr("当前余额: %1 元").arg(QString::number(bal,'f',2)));
        m_statusLabel->setText(tr("充值成功: +%1 元").arg(QString::number(amount,'f',2)));
        
        // 显示充值成功对话框
        auto *successDialog = new PaymentSuccessDialog(amount, bal, this);
        successDialog->exec();
        successDialog->deleteLater();
    });
    connect(m_service, &EvaluateService::rechargeFailed, this, [this](const QString& err){
        m_statusLabel->setText(tr("充值失败: %1").arg(err));
    });

    // 请求配置
    requestConfig();
}

void EvaluatePage::requestConfig(){
    m_service->fetchConfig(m_patientName);
}

void EvaluatePage::sendJson(const QJsonObject &){ /* 已服务化，不再直发 */ }

void EvaluatePage::doRecharge(){
    double amount = m_amountSpin->value();
    if (amount <= 0) {
        QMessageBox::warning(this, tr("警告"), tr("请输入有效的充值金额"));
        return;
    }
    
    // 显示二维码对话框
    auto *qrDialog = new QRCodeDialog(amount, this);
    
    // 连接支付完成信号
    connect(qrDialog, &QRCodeDialog::paymentCompleted, this, [this](double amount) {
        // 实际发送充值请求到服务器
        m_service->recharge(m_patientName, amount);
        m_statusLabel->setText(tr("正在确认支付..."));
    });
    
    // 连接支付取消信号
    connect(qrDialog, &QRCodeDialog::paymentCancelled, this, [this]() {
        m_statusLabel->setText(tr("充值已取消"));
    });
    
    qrDialog->exec();
    qrDialog->deleteLater();
}

void EvaluatePage::goBack(){ emit requestBackHome(); }

void EvaluatePage::onScaleSelected() {
    int row = m_scaleList->currentRow();
    if (row >= 0 && row < m_scales.size()) {
        m_currentScaleIndex = row;
        m_scaleStack->setCurrentIndex(row);
        m_resultText->clear();
        m_statusLabel->setText(tr("已选择量表: %1").arg(m_scales[row].name));
    }
}

void EvaluatePage::submitScale() {
    if (m_currentScaleIndex < 0 || m_currentScaleIndex >= m_scales.size()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个量表"));
        return;
    }
    calculateAndShowResult();
}

void EvaluatePage::resetScale() {
    if (m_currentScaleIndex < 0 || m_currentScaleIndex >= m_scaleButtonGroups.size()) return;
    
    const QList<QButtonGroup*> &questionGroups = m_scaleButtonGroups[m_currentScaleIndex];
    for (QButtonGroup *group : questionGroups) {
        for (auto *btn : group->buttons()) {
            btn->setChecked(false);
        }
    }
    m_resultText->clear();
    m_statusLabel->setText(tr("已重置量表"));
}

void EvaluatePage::initializeScales() {
    // PHQ-9 抑郁症筛查量表
    Scale phq9;
    phq9.code = "phq9";
    phq9.name = "PHQ-9 抑郁症筛查";
    phq9.description = "患者健康问卷抑郁量表，用于筛查抑郁症状";
    
    QStringList phq9Options = {"从不", "几天", "一半以上的天数", "几乎每天"};
    QList<int> phq9Scores = {0, 1, 2, 3};
    
    phq9.questions = {
        {"做事时提不起劲或没有兴趣", phq9Options, phq9Scores},
        {"感到心情低落、沮丧或绝望", phq9Options, phq9Scores},
        {"入睡困难、睡不安稳或睡眠过多", phq9Options, phq9Scores},
        {"感觉疲倦或没有活力", phq9Options, phq9Scores},
        {"食欲不振或吃太多", phq9Options, phq9Scores},
        {"觉得自己很糟糕，或觉得自己很失败，或让自己或家人失望", phq9Options, phq9Scores},
        {"对事物专注有困难，例如阅读报纸或看电视时", phq9Options, phq9Scores},
        {"动作或说话速度缓慢到别人已经察觉，或正好相反—烦躁或坐立不安、动来动去的情况更胜于平常", phq9Options, phq9Scores},
        {"有不如死掉或用某种方式伤害自己的念头", phq9Options, phq9Scores}
    };
    
    phq9.interpretation = "评分说明：0-4分：无或极轻微抑郁；5-9分：轻度抑郁；10-14分：中度抑郁；15-19分：中重度抑郁；20-27分：重度抑郁";
    
    // GAD-7 焦虑量表
    Scale gad7;
    gad7.code = "gad7";
    gad7.name = "GAD-7 焦虑症筛查";
    gad7.description = "广泛性焦虑障碍量表，用于评估焦虑症状";
    
    QStringList gad7Options = {"完全没有", "几天", "一半以上的天数", "几乎每天"};
    QList<int> gad7Scores = {0, 1, 2, 3};
    
    gad7.questions = {
        {"感到紧张、焦虑或烦躁", gad7Options, gad7Scores},
        {"无法停止或控制担忧", gad7Options, gad7Scores},
        {"对各种各样的事情担忧过多", gad7Options, gad7Scores},
        {"很难放松下来", gad7Options, gad7Scores},
        {"坐立不安，难以安静地坐着", gad7Options, gad7Scores},
        {"变得容易烦恼或易怒", gad7Options, gad7Scores},
        {"害怕会有可怕的事情发生", gad7Options, gad7Scores}
    };
    
    gad7.interpretation = "评分说明：0-4分：无焦虑；5-9分：轻度焦虑；10-14分：中度焦虑；15-21分：重度焦虑";
    
    // 简化版生活质量量表
    Scale qol;
    qol.code = "qol";
    qol.name = "生活质量评估";
    qol.description = "简化版生活质量评估，关注日常生活各个方面";
    
    QStringList qolOptions = {"很差", "较差", "一般", "较好", "很好"};
    QList<int> qolScores = {1, 2, 3, 4, 5};
    
    qol.questions = {
        {"您如何评价您的整体健康状况？", qolOptions, qolScores},
        {"您对自己的睡眠质量满意吗？", qolOptions, qolScores},
        {"您有足够的精力应对日常生活吗？", qolOptions, qolScores},
        {"您对自己的人际关系满意吗？", qolOptions, qolScores},
        {"您对自己的工作能力满意吗？", qolOptions, qolScores},
        {"您对自己的生活环境满意吗？", qolOptions, qolScores}
    };
    
    qol.interpretation = "评分说明：6-12分：生活质量较差；13-18分：生活质量一般；19-24分：生活质量较好；25-30分：生活质量很好";
    
    m_scales = {phq9, gad7, qol};
    
    // 创建UI
    for (const Scale &scale : m_scales) {
        m_scaleList->addItem(scale.name);
        createScaleWidget(scale);
    }
    
    if (!m_scales.isEmpty()) {
        m_scaleList->setCurrentRow(0);
        onScaleSelected();
    }
}

void EvaluatePage::createScaleWidget(const Scale &scale) {
    auto *scrollArea = new QScrollArea();
    auto *widget = new QWidget();
    auto *layout = new QVBoxLayout(widget);
    
    // 量表描述
    auto *descLabel = new QLabel(QString("<h3>%1</h3><p>%2</p>").arg(scale.name, scale.description));
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    // 为当前量表创建按钮组列表
    QList<QButtonGroup*> scaleGroups;
    
    // 添加问题
    for (int i = 0; i < scale.questions.size(); ++i) {
        const Question &q = scale.questions[i];
        
        auto *questionLabel = new QLabel(QString("<b>%1. %2</b>").arg(i + 1).arg(q.text));
        questionLabel->setWordWrap(true);
        layout->addWidget(questionLabel);
        
        // 为每个问题创建独立的按钮组
        auto *questionGroup = new QButtonGroup(this);
        scaleGroups.append(questionGroup);
        
        auto *optionsLayout = new QVBoxLayout();
        for (int j = 0; j < q.options.size(); ++j) {
            auto *radio = new QRadioButton(q.options[j]);
            radio->setProperty("questionIndex", i);
            radio->setProperty("optionIndex", j);
            radio->setProperty("score", q.scores[j]);
            questionGroup->addButton(radio, j); // 按钮在组内的ID
            optionsLayout->addWidget(radio);
        }
        layout->addLayout(optionsLayout);
        layout->addSpacing(10);
    }
    
    // 保存当前量表的所有按钮组
    m_scaleButtonGroups.append(scaleGroups);
    
    // 评分说明
    auto *interpLabel = new QLabel(QString("<i>%1</i>").arg(scale.interpretation));
    interpLabel->setWordWrap(true);
    interpLabel->setStyleSheet("color: #666;");
    layout->addWidget(interpLabel);
    
    layout->addStretch();
    scrollArea->setWidget(widget);
    scrollArea->setWidgetResizable(true);
    m_scaleStack->addWidget(scrollArea);
}

void EvaluatePage::calculateAndShowResult() {
    if (m_currentScaleIndex < 0 || m_currentScaleIndex >= m_scales.size()) return;
    
    const Scale &scale = m_scales[m_currentScaleIndex];
    const QList<QButtonGroup*> &questionGroups = m_scaleButtonGroups[m_currentScaleIndex];
    
    int totalScore = 0;
    int answeredQuestions = 0;
    
    // 遍历每个问题的按钮组
    for (int i = 0; i < questionGroups.size(); ++i) {
        QButtonGroup *group = questionGroups[i];
        bool questionAnswered = false;
        
        for (auto *button : group->buttons()) {
            if (button->isChecked()) {
                int score = button->property("score").toInt();
                totalScore += score;
                questionAnswered = true;
                break;
            }
        }
        
        if (questionAnswered) {
            answeredQuestions++;
        }
    }
    
    if (answeredQuestions < scale.questions.size()) {
        QMessageBox::warning(this, tr("警告"), tr("请完成所有题目后再提交"));
        return;
    }
    
    QString result = QString("<h3>%1 评估结果</h3>").arg(scale.name);
    result += QString("<p><b>总分：%1</b></p>").arg(totalScore);
    result += QString("<p>%1</p>").arg(scale.interpretation);
    
    // 添加具体建议
    if (scale.code == "phq9") {
        if (totalScore <= 4) result += "<p><b>建议：</b>保持良好的心理状态</p>";
        else if (totalScore <= 9) result += "<p><b>建议：</b>注意心理健康，可寻求专业指导</p>";
        else if (totalScore <= 14) result += "<p><b>建议：</b>建议咨询心理医生</p>";
        else result += "<p><b>建议：</b>强烈建议寻求专业心理治疗</p>";
    } else if (scale.code == "gad7") {
        if (totalScore <= 4) result += "<p><b>建议：</b>焦虑水平正常</p>";
        else if (totalScore <= 9) result += "<p><b>建议：</b>适当放松，注意压力管理</p>";
        else if (totalScore <= 14) result += "<p><b>建议：</b>建议咨询专业医生</p>";
        else result += "<p><b>建议：</b>需要专业治疗</p>";
    } else if (scale.code == "qol") {
        if (totalScore >= 25) result += "<p><b>建议：</b>生活质量很好，继续保持</p>";
        else if (totalScore >= 19) result += "<p><b>建议：</b>生活质量较好</p>";
        else if (totalScore >= 13) result += "<p><b>建议：</b>可以改善生活方式</p>";
        else result += "<p><b>建议：</b>建议关注健康，改善生活质量</p>";
    }
    
    m_resultText->setHtml(result);
    m_statusLabel->setText(tr("评估完成"));
    
    // TODO: 保存结果到数据库
}

void EvaluatePage::handleResponse(const QJsonObject &){ /* 已服务化，兼容入口保留为空实现 */ }
