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
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(12);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("evaluateTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("评估与充值", topBar);
    title->setObjectName("evaluateTitle");
    QLabel* subTitle = new QLabel("健康评估 & 账户充值", topBar);
    subTitle->setObjectName("evaluateSubTitle");
    QVBoxLayout* titleV = new QVBoxLayout();
    titleV->setContentsMargins(0,0,0,0);
    titleV->addWidget(title);
    titleV->addWidget(subTitle);
    topBarLayout->addLayout(titleV);
    topBarLayout->addStretch();
    mainLay->addWidget(topBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(15);

    // 余额显示
    m_balanceLabel = new QLabel(tr("当前余额: 加载中...")); 
    contentLayout->addWidget(m_balanceLabel);

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
    contentLayout->addWidget(rechargeBox);

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
    contentLayout->addWidget(scaleBox);

    // 状态标签
    m_statusLabel = new QLabel(); 
    contentLayout->addWidget(m_statusLabel);

    // 返回按钮
    m_backBtn = new QPushButton(tr("返回首页"));
    connect(m_backBtn, &QPushButton::clicked, this, &EvaluatePage::goBack);
    contentLayout->addWidget(m_backBtn);
    
    mainLay->addWidget(contentWidget);

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
    
    // DASS-21 抑郁焦虑压力量表（简化版）
    Scale dass21;
    dass21.code = "dass21";
    dass21.name = "DASS-21 抑郁焦虑压力评估";
    dass21.description = "抑郁、焦虑、压力量表，用于评估三种负面情绪状态";
    
    QStringList dassOptions = {"从不适用", "有时适用", "经常适用", "总是适用"};
    QList<int> dassScores = {0, 1, 2, 3};
    
    dass21.questions = {
        {"我发现很难放松下来", dassOptions, dassScores},
        {"我感到口干舌燥", dassOptions, dassScores},
        {"我似乎无法体验到任何积极的感受", dassOptions, dassScores},
        {"我感到呼吸困难（如过度换气、无运动时气喘）", dassOptions, dassScores},
        {"我发现很难主动去做事情", dassOptions, dassScores},
        {"我倾向于对事情反应过度", dassOptions, dassScores},
        {"我感到颤抖（如手抖）", dassOptions, dassScores},
        {"我感到自己消耗了很多神经能量", dassOptions, dassScores},
        {"我担心一些可能让我恐慌或出洋相的场合", dassOptions, dassScores},
        {"我觉得自己没有什么可期待的", dassOptions, dassScores},
        {"我发现自己很容易心烦意乱", dassOptions, dassScores},
        {"我发现很难安静下来", dassOptions, dassScores},
        {"我感到沮丧和忧郁", dassOptions, dassScores},
        {"我对任何阻碍我正在做的事情的因素都无法容忍", dassOptions, dassScores},
        {"我感到快要恐慌了", dassOptions, dassScores},
        {"我对任何事情都无法兴奋起来", dassOptions, dassScores},
        {"我觉得自己作为一个人没有多大价值", dassOptions, dassScores},
        {"我发现自己相当敏感", dassOptions, dassScores},
        {"我感到心跳加快，但并非因为体力消耗", dassOptions, dassScores},
        {"我无缘无故地感到害怕", dassOptions, dassScores},
        {"我觉得生活没有意义", dassOptions, dassScores}
    };
    
    dass21.interpretation = "评分说明：抑郁(3,5,10,13,16,17,21题)：0-9正常，10-13轻度，14-20中度，21-27重度；焦虑(2,4,7,9,15,19,20题)：0-7正常，8-9轻度，10-14中度，15-19重度；压力(1,6,8,11,12,14,18题)：0-14正常，15-18轻度，19-25中度，26-33重度";
    
    // PSQI 匹兹堡睡眠质量指数（简化版）
    Scale psqi;
    psqi.code = "psqi";
    psqi.name = "PSQI 睡眠质量评估";
    psqi.description = "匹兹堡睡眠质量指数，评估睡眠质量和睡眠障碍";
    
    QStringList psqiOptions1 = {"非常好", "相当好", "相当差", "非常差"};
    QList<int> psqiScores1 = {0, 1, 2, 3};
    
    QStringList psqiOptions2 = {"从未", "少于1次/周", "1-2次/周", "≥3次/周"};
    QList<int> psqiScores2 = {0, 1, 2, 3};
    
    psqi.questions = {
        {"您如何评价您的整体睡眠质量？", psqiOptions1, psqiScores1},
        {"您入睡需要多长时间？", {"≤15分钟", "16-30分钟", "31-60分钟", ">60分钟"}, {0, 1, 2, 3}},
        {"您实际睡眠时间有多长？", {">7小时", "6-7小时", "5-6小时", "<5小时"}, {0, 1, 2, 3}},
        {"您多久一次因为疼痛而睡眠困难？", psqiOptions2, psqiScores2},
        {"您多久一次因为咳嗽或打鼾而睡眠困难？", psqiOptions2, psqiScores2},
        {"您多久一次因为感到太冷而睡眠困难？", psqiOptions2, psqiScores2},
        {"您多久一次因为感到太热而睡眠困难？", psqiOptions2, psqiScores2},
        {"您多久一次因为做恶梦而睡眠困难？", psqiOptions2, psqiScores2},
        {"您多久一次需要服用药物才能入睡？", psqiOptions2, psqiScores2}
    };
    
    psqi.interpretation = "评分说明：总分0-21分，得分≤7分表示睡眠质量较好，>7分表示睡眠质量较差，分数越高睡眠质量越差";
    
    // AUDIT 酒精使用障碍识别测试（简化版）
    Scale audit;
    audit.code = "audit";
    audit.name = "AUDIT 酒精使用评估";
    audit.description = "酒精使用障碍识别测试，用于筛查酒精相关问题";
    
    audit.questions = {
        {"您多久喝一次酒？", {"从不", "每月1次或更少", "每月2-4次", "每周2-3次", "每周4次或更多"}, {0, 1, 2, 3, 4}},
        {"您平时一次喝多少酒？", {"1-2杯", "3-4杯", "5-6杯", "7-9杯", "10杯或更多"}, {0, 1, 2, 3, 4}},
        {"您多久一次在一个场合喝6杯或更多？", {"从不", "少于每月1次", "每月1次", "每周1次", "每天或几乎每天"}, {0, 1, 2, 3, 4}},
        {"过去一年您多久一次发现一旦开始喝酒就停不下来？", {"从不", "少于每月1次", "每月1次", "每周1次", "每天或几乎每天"}, {0, 1, 2, 3, 4}},
        {"过去一年您多久一次因为喝酒而没能做您正常应该做的事？", {"从不", "少于每月1次", "每月1次", "每周1次", "每天或几乎每天"}, {0, 1, 2, 3, 4}},
        {"过去一年您多久一次在大量饮酒后需要早晨喝酒来振作精神？", {"从不", "少于每月1次", "每月1次", "每周1次", "每天或几乎每天"}, {0, 1, 2, 3, 4}},
        {"过去一年您多久一次因为喝酒感到内疚或后悔？", {"从不", "少于每月1次", "每月1次", "每周1次", "每天或几乎每天"}, {0, 1, 2, 3, 4}},
        {"过去一年您多久一次因为喝酒而记不起前一晚发生的事？", {"从不", "少于每月1次", "每月1次", "每周1次", "每天或几乎每天"}, {0, 1, 2, 3, 4}},
        {"您或其他人是否曾因您的饮酒而受伤？", {"从未", "有，但不在过去一年", "有，在过去一年"}, {0, 2, 4}},
        {"亲戚、朋友、医生或其他保健人员是否曾关心您的饮酒或建议您少喝？", {"从未", "有，但不在过去一年", "有，在过去一年"}, {0, 2, 4}}
    };
    
    audit.interpretation = "评分说明：0-7分：低风险饮酒；8-15分：危险饮酒；16-19分：有害饮酒；20-40分：可能酒精依赖";
    
    // SF-12 健康调查简表
    Scale sf12;
    sf12.code = "sf12";
    sf12.name = "SF-12 健康状况调查";
    sf12.description = "简版健康调查表，评估身体和心理健康状况";
    
    sf12.questions = {
        {"总的来说，您认为您的健康状况如何？", {"极好", "很好", "好", "一般", "差"}, {5, 4, 3, 2, 1}},
        {"以下哪项最好地描述了您的健康状况限制您进行中等程度活动的情况？", {"完全不受限制", "受到一点限制", "受到很大限制"}, {3, 2, 1}},
        {"您的健康状况限制您爬几层楼梯吗？", {"完全不受限制", "受到一点限制", "受到很大限制"}, {3, 2, 1}},
        {"在过去4周内，由于身体健康问题，您工作或其他日常活动时间减少了吗？", {"没有", "有一点", "相当多"}, {3, 2, 1}},
        {"在过去4周内，由于身体健康问题，您完成工作或其他活动的能力下降了吗？", {"没有", "有一点", "相当多"}, {3, 2, 1}},
        {"在过去4周内，由于情绪问题，您工作或其他日常活动时间减少了吗？", {"没有", "有一点", "相当多"}, {3, 2, 1}},
        {"在过去4周内，由于情绪问题，您完成工作或其他活动时不如往常仔细吗？", {"没有", "有一点", "相当多"}, {3, 2, 1}},
        {"在过去4周内，疼痛对您的正常工作（包括家务和外出工作）妨碍程度如何？", {"完全没有妨碍", "有一点妨碍", "中等程度妨碍", "相当妨碍", "极度妨碍"}, {5, 4, 3, 2, 1}},
        {"在过去4周内，您感到平静和安宁的时间有多少？", {"所有时间", "大部分时间", "一些时间", "少量时间", "没有时间"}, {5, 4, 3, 2, 1}},
        {"在过去4周内，您精力充沛的时间有多少？", {"所有时间", "大部分时间", "一些时间", "少量时间", "没有时间"}, {5, 4, 3, 2, 1}},
        {"在过去4周内，您感到沮丧和忧郁的时间有多少？", {"没有时间", "少量时间", "一些时间", "大部分时间", "所有时间"}, {5, 4, 3, 2, 1}},
        {"在过去4周内，您的身体健康或情绪问题妨碍您的社交活动（如拜访朋友、亲戚等）的时间有多少？", {"没有时间", "少量时间", "一些时间", "大部分时间", "所有时间"}, {5, 4, 3, 2, 1}}
    };
    
    sf12.interpretation = "评分说明：12-28分：健康状况差；29-40分：健康状况一般；41-52分：健康状况良好；53-60分：健康状况优秀";
    
    m_scales = {phq9, gad7, qol, dass21, psqi, audit, sf12};
    
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
    } else if (scale.code == "dass21") {
        // DASS-21需要分别计算抑郁、焦虑、压力三个维度
        result += "<p><b>注意：</b>此量表需要分维度评估，总分仅供参考</p>";
        if (totalScore <= 21) result += "<p><b>建议：</b>整体情绪状态良好</p>";
        else if (totalScore <= 42) result += "<p><b>建议：</b>注意情绪调节，适当减压</p>";
        else result += "<p><b>建议：</b>建议寻求专业心理咨询</p>";
    } else if (scale.code == "psqi") {
        if (totalScore <= 7) result += "<p><b>建议：</b>睡眠质量良好，继续保持</p>";
        else if (totalScore <= 14) result += "<p><b>建议：</b>睡眠质量一般，注意改善睡眠习惯</p>";
        else result += "<p><b>建议：</b>睡眠质量差，建议咨询医生</p>";
    } else if (scale.code == "audit") {
        if (totalScore <= 7) result += "<p><b>建议：</b>饮酒风险低，保持适度</p>";
        else if (totalScore <= 15) result += "<p><b>建议：</b>危险饮酒，建议减少饮酒</p>";
        else if (totalScore <= 19) result += "<p><b>建议：</b>有害饮酒，强烈建议控制饮酒</p>";
        else result += "<p><b>建议：</b>可能酒精依赖，建议寻求专业帮助</p>";
    } else if (scale.code == "sf12") {
        if (totalScore >= 53) result += "<p><b>建议：</b>健康状况优秀，继续保持</p>";
        else if (totalScore >= 41) result += "<p><b>建议：</b>健康状况良好</p>";
        else if (totalScore >= 29) result += "<p><b>建议：</b>健康状况一般，注意改善</p>";
        else result += "<p><b>建议：</b>健康状况需要关注，建议咨询医生</p>";
    }
    
    m_resultText->setHtml(result);
    m_statusLabel->setText(tr("评估完成"));
    
    // TODO: 保存结果到数据库
}

void EvaluatePage::handleResponse(const QJsonObject &){ /* 已服务化，兼容入口保留为空实现 */ }
