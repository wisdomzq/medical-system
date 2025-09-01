#include "remotedatawidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QTextStream>
#include <QFile>
#include <QSplitter>

RemoteDataWidget::RemoteDataWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), isCollecting_(false), dataCount_(0) {
    setupUI();
    dataTimer_ = new QTimer(this);
    connect(dataTimer_, &QTimer::timeout, this, &RemoteDataWidget::onDataReceived);

    // 加载专用样式
    QFile qss(":/doctor_remotedata.qss");
    if (qss.open(QFile::ReadOnly)) {
        this->setStyleSheet(QString::fromLatin1(qss.readAll()));
    }
}

RemoteDataWidget::~RemoteDataWidget() = default;

void RemoteDataWidget::setupUI() {
    setWindowTitle("远程数据采集分析 - " + doctorName_);
    
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(10, 8, 10, 8); // 更紧凑的页面边距
    mainLayout_->setSpacing(8); // 缩小区块间距

    // 顶部栏
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("remoteTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* tb = new QHBoxLayout(topBar);
    tb->setContentsMargins(12, 8, 12, 8); // 顶部栏更紧凑
    QVBoxLayout* tcol = new QVBoxLayout();
    tcol->setContentsMargins(0,0,0,0);
    QLabel* title = new QLabel("远程数据采集", topBar);
    title->setObjectName("remoteTitle");
    QLabel* sub = new QLabel("设备控制与数据分析", topBar);
    sub->setObjectName("remoteSubTitle");
    tcol->addWidget(title);
    tcol->addWidget(sub);
    tb->addLayout(tcol);
    tb->addStretch();
    mainLayout_->addWidget(topBar);
    
    setupDeviceInfo();
    setupDataDisplay();
    setupAnalysisPanel();
    
    setLayout(mainLayout_);
}

void RemoteDataWidget::setupDeviceInfo() {
    deviceGroup_ = new QGroupBox("智能心电仪设备控制");
    deviceGroup_->setObjectName("deviceCard");
    deviceGroup_->setAttribute(Qt::WA_StyledBackground, true);
    auto deviceLayout = new QVBoxLayout(deviceGroup_);
    deviceLayout->setContentsMargins(14, 12, 14, 12);
    deviceLayout->setSpacing(8);
    
    // 设备选择和连接
    auto deviceSelectLayout = new QHBoxLayout();
    {
        QLabel* lbl = new QLabel("设备选择:");
        lbl->setObjectName("formLabel");
        deviceSelectLayout->addWidget(lbl);
    }
    
    deviceCombo_ = new QComboBox();
    deviceCombo_->addItems({"心电仪-001 (192.168.1.100)", 
                           "心电仪-002 (192.168.1.101)", 
                           "心电仪-003 (192.168.1.102)"});
    deviceSelectLayout->addWidget(deviceCombo_);
    
    refreshDeviceBtn_ = new QPushButton("刷新设备");
    refreshDeviceBtn_->setObjectName("outlineBtn");
    connect(refreshDeviceBtn_, &QPushButton::clicked, this, &RemoteDataWidget::refreshDeviceList);
    deviceSelectLayout->addWidget(refreshDeviceBtn_);
    
    deviceSelectLayout->addStretch();
    deviceLayout->addLayout(deviceSelectLayout);
    
    // 设备IP和采样率设置
    auto settingsLayout = new QHBoxLayout();
    {
        QLabel* lbl = new QLabel("设备IP:");
        lbl->setObjectName("formLabel");
        settingsLayout->addWidget(lbl);
    }
    
    deviceIPEdit_ = new QLineEdit("192.168.1.100");
    deviceIPEdit_->setMaximumWidth(120);
    settingsLayout->addWidget(deviceIPEdit_);
    
    {
        QLabel* lbl = new QLabel("采样率(Hz):");
        lbl->setObjectName("formLabel");
        settingsLayout->addWidget(lbl);
    }
    sampleRateSpinBox_ = new QSpinBox();
    sampleRateSpinBox_->setRange(100, 1000);
    sampleRateSpinBox_->setValue(250);
    sampleRateSpinBox_->setMaximumWidth(80);
    settingsLayout->addWidget(sampleRateSpinBox_);
    
    settingsLayout->addStretch();
    deviceLayout->addLayout(settingsLayout);
    
    // 控制按钮和状态
    auto controlLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton("开始采集");
    startBtn_->setObjectName("primaryBtn");
    connect(startBtn_, &QPushButton::clicked, this, &RemoteDataWidget::startDataCollection);
    controlLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton("停止采集");
    stopBtn_->setObjectName("dangerBtn");
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &RemoteDataWidget::stopDataCollection);
    controlLayout->addWidget(stopBtn_);
    
    deviceStatusLabel_ = new QLabel("设备未连接");
    deviceStatusLabel_->setObjectName("statusLabel");
    deviceStatusLabel_->setProperty("status", "err");
    controlLayout->addWidget(deviceStatusLabel_);
    
    controlLayout->addStretch();
    
    connectionProgress_ = new QProgressBar();
    connectionProgress_->setVisible(false);
    connectionProgress_->setMaximumWidth(160);
    controlLayout->addWidget(connectionProgress_);
    
    deviceLayout->addLayout(controlLayout);
    
    mainLayout_->addWidget(deviceGroup_);
}

void RemoteDataWidget::setupDataDisplay() {
    dataGroup_ = new QGroupBox("实时数据监控");
    dataGroup_->setObjectName("dataCard");
    dataGroup_->setAttribute(Qt::WA_StyledBackground, true);
    auto dataLayout = new QVBoxLayout(dataGroup_);
    dataLayout->setContentsMargins(14, 12, 14, 12);
    dataLayout->setSpacing(6);
    
    // 数据统计信息
    auto statsLayout = new QHBoxLayout();
    dataCountLabel_ = new QLabel("采集数据量: 0 条");
    dataCountLabel_->setObjectName("infoLabel");
    statsLayout->addWidget(dataCountLabel_);
    statsLayout->addStretch();
    
    dataLayout->addLayout(statsLayout);
    
    // 原始数据显示
    rawDataEdit_ = new QTextEdit();
    rawDataEdit_->setObjectName("rawData");
    rawDataEdit_->setMaximumHeight(110); // 适度压缩，但保留可读性
    rawDataEdit_->setPlaceholderText("心电数据将在这里实时显示...");
    dataLayout->addWidget(rawDataEdit_);
    
    mainLayout_->addWidget(dataGroup_);
}

void RemoteDataWidget::setupAnalysisPanel() {
    analysisGroup_ = new QGroupBox("数据分析结果");
    analysisGroup_->setObjectName("analysisCard");
    analysisGroup_->setAttribute(Qt::WA_StyledBackground, true);
    auto analysisLayout = new QVBoxLayout(analysisGroup_);
    analysisLayout->setContentsMargins(14, 12, 14, 12);
    analysisLayout->setSpacing(8);
    
    // 分析控制按钮
    auto analysisControlLayout = new QHBoxLayout();
    
    analyzeBtn_ = new QPushButton("开始分析");
    analyzeBtn_->setObjectName("primaryBtn");
    connect(analyzeBtn_, &QPushButton::clicked, this, &RemoteDataWidget::analyzeData);
    analysisControlLayout->addWidget(analyzeBtn_);
    
    exportBtn_ = new QPushButton("导出报告");
    exportBtn_->setObjectName("exportBtn");
    connect(exportBtn_, &QPushButton::clicked, this, &RemoteDataWidget::exportAnalysisReport);
    analysisControlLayout->addWidget(exportBtn_);
    
    clearBtn_ = new QPushButton("清空历史");
    clearBtn_->setObjectName("dangerBtn");
    connect(clearBtn_, &QPushButton::clicked, this, &RemoteDataWidget::clearHistory);
    analysisControlLayout->addWidget(clearBtn_);
    
    analysisControlLayout->addStretch();
    analysisLayout->addLayout(analysisControlLayout);
    
    // 分析结果区域：左右排列（可拖拽调整）
    QSplitter* splitter = new QSplitter(Qt::Horizontal, analysisGroup_);

    // 左侧：分析结果表格
    analysisTable_ = new QTableWidget(0, 4);
    analysisTable_->setObjectName("analysisTable");
    analysisTable_->setHorizontalHeaderLabels({"时间", "心率(BPM)", "状态", "异常标记"});
    analysisTable_->horizontalHeader()->setStretchLastSection(true);
    analysisTable_->verticalHeader()->setVisible(false); // 隐藏行号减少占高
    analysisTable_->verticalHeader()->setDefaultSectionSize(20); // 降低行高
    analysisTable_->setAlternatingRowColors(true);
    analysisTable_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    splitter->addWidget(analysisTable_);

    // 右侧：详细分析报告
    analysisResultEdit_ = new QTextEdit();
    analysisResultEdit_->setPlaceholderText("详细的心电分析报告将在这里显示...");
    analysisResultEdit_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    splitter->addWidget(analysisResultEdit_);

    // 初始比例：表格略宽
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    analysisLayout->addWidget(splitter);
    
    mainLayout_->addWidget(analysisGroup_);
}

void RemoteDataWidget::startDataCollection() {
    isCollecting_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    
    deviceStatusLabel_->setText("正在连接设备...");
    deviceStatusLabel_->setProperty("status", "warn");
    
    connectionProgress_->setVisible(true);
    connectionProgress_->setRange(0, 100);
    
    // 模拟连接过程
    QTimer* connectTimer = new QTimer(this);
    int progress = 0;
    connect(connectTimer, &QTimer::timeout, [this, connectTimer, &progress]() {
        progress += 10;
        connectionProgress_->setValue(progress);
        
        if (progress >= 100) {
            connectTimer->stop();
            connectTimer->deleteLater();
            connectionProgress_->setVisible(false);
            
            deviceStatusLabel_->setText("设备已连接，正在采集数据");
            deviceStatusLabel_->setProperty("status", "ok");
            
            // 开始模拟数据采集
            dataTimer_->start(500); // 每500ms接收一次数据
        }
    });
    connectTimer->start(200);
}

void RemoteDataWidget::stopDataCollection() {
    isCollecting_ = false;
    dataTimer_->stop();
    
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    
    deviceStatusLabel_->setText("数据采集已停止");
    deviceStatusLabel_->setProperty("status", "err");
    
    QMessageBox::information(this, "信息", QString("数据采集已停止。共采集了 %1 条数据。").arg(dataCount_));
}

void RemoteDataWidget::onDataReceived() {
    if (!isCollecting_) return;
    
    simulateECGData();
    dataCount_++;
    dataCountLabel_->setText(QString("采集数据量: %1 条").arg(dataCount_));
}

void RemoteDataWidget::simulateECGData() {
    // 模拟心电数据生成
    QDateTime currentTime = QDateTime::currentDateTime();
    
    // 生成模拟的心电信号数据点
    QStringList dataPoints;
    for (int i = 0; i < 10; i++) {
        // 模拟心电波形数据 (-1000 到 1000 范围)
        int value = QRandomGenerator::global()->bounded(-1000, 1000);
        
        // 添加一些典型的心电波形特征
        if (i == 3 || i == 4) { // R波峰
            value = QRandomGenerator::global()->bounded(800, 1000);
        } else if (i == 2 || i == 5) { // Q波和S波
            value = QRandomGenerator::global()->bounded(-200, -100);
        }
        
        dataPoints.append(QString::number(value));
    }
    
    QString ecgData = QString("[%1] ECG: %2")
                      .arg(currentTime.toString("hh:mm:ss.zzz"))
                      .arg(dataPoints.join(", "));
    
    collectedData_.append(ecgData);
    updateDataDisplay(ecgData);
}

void RemoteDataWidget::updateDataDisplay(const QString& data) {
    rawDataEdit_->append(data);
    
    // 限制显示的数据条数，避免界面卡顿
    if (rawDataEdit_->document()->blockCount() > 100) {
        QTextCursor cursor = rawDataEdit_->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // 删除换行符
    }
    
    // 自动滚动到底部
    QTextCursor cursor = rawDataEdit_->textCursor();
    cursor.movePosition(QTextCursor::End);
    rawDataEdit_->setTextCursor(cursor);
}

void RemoteDataWidget::analyzeData() {
    if (collectedData_.isEmpty()) {
        QMessageBox::warning(this, "警告", "没有可分析的数据，请先采集数据！");
        return;
    }
    
    performDataAnalysis();
}

void RemoteDataWidget::performDataAnalysis() {
    analysisResultEdit_->clear();
    analysisTable_->setRowCount(0);
    
    analysisResultEdit_->append("=== 心电数据分析报告 ===");
    analysisResultEdit_->append(QString("分析时间: %1").arg(QDateTime::currentDateTime().toString()));
    analysisResultEdit_->append(QString("医生: %1").arg(doctorName_));
    analysisResultEdit_->append(QString("数据量: %1 条").arg(collectedData_.size()));
    analysisResultEdit_->append("");
    
    // 模拟分析过程
    int normalCount = 0;
    int abnormalCount = 0;
    
    for (int i = 0; i < qMin(collectedData_.size(), 20); i++) {
        // 模拟心率分析
        int heartRate = QRandomGenerator::global()->bounded(60, 120);
        QString status = "正常";
        QString abnormalMark = "-";
        
        // 模拟异常检测
        if (QRandomGenerator::global()->bounded(100) < 15) { // 15%概率异常
            if (heartRate < 70) {
                status = "心动过缓";
                abnormalMark = "⚠";
                abnormalCount++;
            } else {
                status = "心动过速";
                abnormalMark = "⚠";
                abnormalCount++;
            }
        } else {
            normalCount++;
        }
        
        // 添加到表格
        int row = analysisTable_->rowCount();
        analysisTable_->insertRow(row);
        analysisTable_->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().addSecs(-i*30).toString("hh:mm:ss")));
        analysisTable_->setItem(row, 1, new QTableWidgetItem(QString::number(heartRate)));
        analysisTable_->setItem(row, 2, new QTableWidgetItem(status));
        analysisTable_->setItem(row, 3, new QTableWidgetItem(abnormalMark));
        
        // 异常行高亮
        if (abnormalMark != "-") {
            for (int col = 0; col < 4; col++) {
                analysisTable_->item(row, col)->setBackground(QBrush(QColor(255, 240, 240)));
            }
        }
    }
    
    // 生成分析总结
    analysisResultEdit_->append("=== 分析结果总结 ===");
    analysisResultEdit_->append(QString("正常心律: %1 次 (%2%)")
                                .arg(normalCount)
                                .arg(QString::number(normalCount * 100.0 / (normalCount + abnormalCount), 'f', 1)));
    analysisResultEdit_->append(QString("异常心律: %1 次 (%2%)")
                                .arg(abnormalCount)
                                .arg(QString::number(abnormalCount * 100.0 / (normalCount + abnormalCount), 'f', 1)));
    analysisResultEdit_->append("");
    analysisResultEdit_->append("=== 建议 ===");
    
    if (abnormalCount > 0) {
        analysisResultEdit_->append("⚠ 检测到异常心律，建议：");
        analysisResultEdit_->append("1. 进行更详细的心电图检查");
        analysisResultEdit_->append("2. 关注患者的症状和体征");
        analysisResultEdit_->append("3. 考虑药物治疗或进一步诊断");
    } else {
        analysisResultEdit_->append("✓ 心律正常，继续常规监护");
    }
    
    QMessageBox::information(this, "分析完成", "心电数据分析已完成！");
}

void RemoteDataWidget::refreshDeviceList() {
    deviceCombo_->clear();
    deviceStatusLabel_->setText("正在搜索设备...");
    deviceStatusLabel_->setProperty("status", "warn");
    
    // 模拟设备搜索
    QTimer::singleShot(2000, [this]() {
        deviceCombo_->addItems({
            "心电仪-001 (192.168.1.100)",
            "心电仪-002 (192.168.1.101)", 
            "心电仪-003 (192.168.1.102)",
            "心电仪-004 (192.168.1.103)"
        });
        
    deviceStatusLabel_->setText("设备列表已更新");
    deviceStatusLabel_->setProperty("status", "info");
        
        QTimer::singleShot(3000, [this]() {
            deviceStatusLabel_->setText("设备未连接");
            deviceStatusLabel_->setProperty("status", "err");
        });
    });
}

void RemoteDataWidget::exportAnalysisReport() {
    if (analysisResultEdit_->toPlainText().isEmpty()) {
        QMessageBox::warning(this, "警告", "没有分析结果可导出，请先进行数据分析！");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "导出分析报告", 
        QString("心电分析报告_%1_%2.txt")
            .arg(doctorName_)
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "文本文件 (*.txt);;所有文件 (*.*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << analysisResultEdit_->toPlainText();
            
            QMessageBox::information(this, "成功", "分析报告已成功导出到：" + fileName);
        } else {
            QMessageBox::critical(this, "错误", "文件保存失败！");
        }
    }
}

void RemoteDataWidget::clearHistory() {
    auto reply = QMessageBox::question(this, "确认", "确定要清空所有历史数据吗？",
                                      QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        collectedData_.clear();
        rawDataEdit_->clear();
        analysisTable_->setRowCount(0);
        analysisResultEdit_->clear();
        dataCount_ = 0;
        dataCountLabel_->setText("采集数据量: 0 条");
        
        QMessageBox::information(this, "完成", "历史数据已清空！");
    }
}
