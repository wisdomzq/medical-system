#include "datachartwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDate>
#include <QDateTime>
#include <QRandomGenerator>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QDebug>
#include <cmath>

DataChartWidget::DataChartWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), maxValue_(0), minValue_(0) {
    setupUI();
    loadSampleData();
}

DataChartWidget::~DataChartWidget() = default;

void DataChartWidget::setupUI() {
    setWindowTitle("基础数据图表 - " + doctorName_);
    
    mainLayout_ = new QVBoxLayout(this);
    
    // 控制面板 - 分成两行以节省垂直空间
    QVBoxLayout* controlVLayout = new QVBoxLayout();
    
    // 第一行：数据类型和时间选择
    QHBoxLayout* firstRowLayout = new QHBoxLayout();
    
    // 数据类型选择
    firstRowLayout->addWidget(new QLabel("数据类型:"));
    dataTypeCombo_ = new QComboBox();
    dataTypeCombo_->addItems({"患者数量", "诊疗次数", "药品使用量", "收入统计", "满意度评分"});
    connect(dataTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &DataChartWidget::onDataTypeChanged);
    firstRowLayout->addWidget(dataTypeCombo_);
    
    firstRowLayout->addSpacing(20); // 添加一些间距
    
    // 时间范围
    firstRowLayout->addWidget(new QLabel("开始日期:"));
    startDateEdit_ = new QDateEdit(QDate::currentDate().addDays(-30));
    startDateEdit_->setCalendarPopup(true);
    startDateEdit_->setMaximumWidth(120);
    firstRowLayout->addWidget(startDateEdit_);
    
    firstRowLayout->addWidget(new QLabel("结束日期:"));
    endDateEdit_ = new QDateEdit(QDate::currentDate());
    endDateEdit_->setCalendarPopup(true);
    endDateEdit_->setMaximumWidth(120);
    firstRowLayout->addWidget(endDateEdit_);
    
    firstRowLayout->addStretch();
    controlVLayout->addLayout(firstRowLayout);
    
    // 第二行：按钮和状态
    QHBoxLayout* secondRowLayout = new QHBoxLayout();
    
    // 按钮
    generateBtn_ = new QPushButton("生成图表");
    generateBtn_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 6px 12px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &DataChartWidget::generateChart);
    secondRowLayout->addWidget(generateBtn_);
    
    refreshBtn_ = new QPushButton("刷新数据");
    refreshBtn_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 6px 12px; }");
    connect(refreshBtn_, &QPushButton::clicked, this, &DataChartWidget::refreshData);
    secondRowLayout->addWidget(refreshBtn_);
    
    exportBtn_ = new QPushButton("导出图表");
    exportBtn_->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 6px 12px; }");
    connect(exportBtn_, &QPushButton::clicked, this, &DataChartWidget::exportChart);
    secondRowLayout->addWidget(exportBtn_);
    
    secondRowLayout->addStretch();
    
    // 状态标签
    statusLabel_ = new QLabel("就绪");
    statusLabel_->setStyleSheet("color: green; font-weight: bold; font-size: 12px;");
    secondRowLayout->addWidget(statusLabel_);
    
    controlVLayout->addLayout(secondRowLayout);
    mainLayout_->addLayout(controlVLayout);
    
    // 添加一个分隔线
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("color: #cccccc;");
    mainLayout_->addWidget(line);
    
    // 为图表绘制留出空间
    mainLayout_->addStretch();
    
    // 添加一个固定大小的绘图区域
    setMinimumSize(800, 650);
    
    setLayout(mainLayout_);
}

void DataChartWidget::loadSampleData() {
    updateChart(dataTypeCombo_->currentText());
}

void DataChartWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    
    if (chartData_.isEmpty()) return;
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawChart(&painter);
}

void DataChartWidget::drawChart(QPainter* painter) {
    if (chartData_.isEmpty()) return;
    
    // 图表区域设置 - 调整为更合适的上边距
    QRect chartRect = rect().adjusted(80, 150, -50, -80);
    
    // 绘制背景
    painter->fillRect(chartRect, QColor(250, 250, 250));
    painter->setPen(QPen(QColor(200, 200, 200), 1));
    painter->drawRect(chartRect);
    
    // 绘制标题 - 调整标题位置到控制面板和图表之间
    painter->setPen(QPen(Qt::black, 1));
    QFont titleFont = painter->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    QString title = QString("医疗数据统计 - %1 (%2)").arg(currentDataType_, doctorName_);
    painter->drawText(rect().adjusted(0, 110, 0, 0), Qt::AlignTop | Qt::AlignHCenter, title);
    
    // 绘制网格线和Y轴标签
    painter->setPen(QPen(QColor(220, 220, 220), 1));
    QFont labelFont = painter->font();
    labelFont.setPointSize(9);
    painter->setFont(labelFont);
    
    int gridLines = 5;
    for (int i = 0; i <= gridLines; i++) {
        int y = chartRect.top() + i * chartRect.height() / gridLines;
        painter->drawLine(chartRect.left(), y, chartRect.right(), y);
        
        // Y轴标签
        double value = maxValue_ - i * (maxValue_ - minValue_) / gridLines;
        painter->setPen(QPen(Qt::black, 1));
        painter->drawText(QRect(10, y - 10, 60, 20), Qt::AlignRight | Qt::AlignVCenter, 
                         QString::number(value, 'f', 1));
        painter->setPen(QPen(QColor(220, 220, 220), 1));
    }
    
    // 绘制数据线
    if (chartData_.size() > 1) {
        painter->setPen(QPen(QColor(33, 150, 243), 2));
        
        QVector<QPointF> points;
        for (int i = 0; i < chartData_.size(); i++) {
            double x = chartRect.left() + (double)i / (chartData_.size() - 1) * chartRect.width();
            double y = chartRect.bottom() - (chartData_[i] - minValue_) / (maxValue_ - minValue_) * chartRect.height();
            points.append(QPointF(x, y));
        }
        
        // 绘制折线
        for (int i = 0; i < points.size() - 1; i++) {
            painter->drawLine(points[i], points[i + 1]);
        }
        
        // 绘制数据点
        painter->setPen(QPen(QColor(255, 87, 34), 1));
        painter->setBrush(QBrush(QColor(255, 152, 0)));
        for (const QPointF& point : points) {
            painter->drawEllipse(point, 3, 3);
        }
    }
    
    // 绘制X轴标签
    painter->setPen(QPen(Qt::black, 1));
    if (chartLabels_.size() > 0) {
        int labelStep = qMax(1, chartLabels_.size() / 10); // 最多显示10个标签
        for (int i = 0; i < chartLabels_.size(); i += labelStep) {
            double x = chartRect.left() + (double)i / (chartData_.size() - 1) * chartRect.width();
            painter->save();
            painter->translate(x, chartRect.bottom() + 20);
            painter->rotate(-45);
            painter->drawText(0, 0, chartLabels_[i]);
            painter->restore();
        }
    }
    
    // 绘制轴标题
    painter->setPen(QPen(Qt::black, 1));
    QFont axisFont = painter->font();
    axisFont.setPointSize(10);
    axisFont.setBold(true);
    painter->setFont(axisFont);
    
    // Y轴标题
    painter->save();
    painter->translate(25, chartRect.center().y());
    painter->rotate(-90);
    painter->drawText(0, 0, currentDataType_);
    painter->restore();
    
    // X轴标题
    painter->drawText(chartRect.center().x() - 20, chartRect.bottom() + 60, "日期");
}

void DataChartWidget::updateChart(const QString& dataType) {
    statusLabel_->setText("正在生成图表...");
    statusLabel_->setStyleSheet("color: orange; font-weight: bold;");
    
    currentDataType_ = dataType;
    chartData_.clear();
    chartLabels_.clear();
    
    // 生成模拟数据
    QDate startDate = startDateEdit_->date();
    QDate endDate = endDateEdit_->date();
    int days = startDate.daysTo(endDate);
    
    if (days <= 0) {
        QMessageBox::warning(this, "警告", "结束日期必须大于开始日期！");
        statusLabel_->setText("数据错误");
        statusLabel_->setStyleSheet("color: red; font-weight: bold;");
        return;
    }
    
    // 根据数据类型生成不同范围的模拟数据
    int baseValue = 50;
    int variance = 20;
    
    if (dataType == "患者数量") {
        baseValue = 30;
        variance = 15;
    } else if (dataType == "诊疗次数") {
        baseValue = 45;
        variance = 18;
    } else if (dataType == "药品使用量") {
        baseValue = 100;
        variance = 30;
    } else if (dataType == "收入统计") {
        baseValue = 5000;
        variance = 1500;
    } else if (dataType == "满意度评分") {
        baseValue = 85;
        variance = 10;
    }
    
    maxValue_ = -999999;
    minValue_ = 999999;
    
    for (int i = 0; i <= days; i++) {
        QDate currentDate = startDate.addDays(i);
        
        // 生成波动数据
        int randomOffset = QRandomGenerator::global()->bounded(-variance, variance);
        double value = baseValue + randomOffset + (sin(i * 0.2) * variance * 0.3);
        
        chartData_.append(value);
        chartLabels_.append(currentDate.toString("MM-dd"));
        
        maxValue_ = qMax(maxValue_, value);
        minValue_ = qMin(minValue_, value);
    }
    
    // 调整Y轴范围，留出一些边距
    double range = maxValue_ - minValue_;
    maxValue_ += range * 0.1;
    minValue_ -= range * 0.1;
    
    statusLabel_->setText("图表生成完成");
    statusLabel_->setStyleSheet("color: green; font-weight: bold;");
    
    // 触发重绘
    update();
}

void DataChartWidget::generateChart() {
    updateChart(dataTypeCombo_->currentText());
}

void DataChartWidget::refreshData() {
    statusLabel_->setText("正在刷新数据...");
    statusLabel_->setStyleSheet("color: blue; font-weight: bold;");
    
    // 模拟数据刷新延迟
    QTimer::singleShot(1000, [this]() {
        updateChart(dataTypeCombo_->currentText());
    });
}

void DataChartWidget::onDataTypeChanged() {
    updateChart(dataTypeCombo_->currentText());
}

void DataChartWidget::exportChart() {
    if (chartData_.isEmpty()) {
        QMessageBox::warning(this, "警告", "没有图表数据可导出！");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "导出图表", 
        QString("医疗数据图表_%1_%2.png").arg(currentDataType_, 
                                                QDate::currentDate().toString("yyyyMMdd")),
        "PNG 图片 (*.png);;JPG 图片 (*.jpg)");
    
    if (!fileName.isEmpty()) {
        // 创建一个更大的图像来保存图表
        QPixmap pixmap(1000, 700);
        pixmap.fill(Qt::white);
        
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 在更大的画布上绘制图表
        QRect oldRect = rect();
        setGeometry(0, 0, 1000, 700);
        drawChart(&painter);
        setGeometry(oldRect);
        
        if (pixmap.save(fileName)) {
            statusLabel_->setText("图表导出成功");
            statusLabel_->setStyleSheet("color: green; font-weight: bold;");
            QMessageBox::information(this, "成功", "图表已成功导出到：" + fileName);
        } else {
            statusLabel_->setText("图表导出失败");
            statusLabel_->setStyleSheet("color: red; font-weight: bold;");
            QMessageBox::critical(this, "错误", "图表导出失败！");
        }
    }
}
