#include "doctorinfopage.h"
#include "core/network/communicationclient.h"
#include <QApplication>
#include <QScreen>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QPixmap>
#include <QGroupBox>
#include <QFont>
#include <QPalette>
#include <QDateTime>
#include <QUuid>
#include <QDebug>

DoctorInfoPage::DoctorInfoPage(const QString& doctorUsername, CommunicationClient* client, QWidget *parent)
    : QWidget(parent), m_doctorUsername(doctorUsername), m_client(client)
{
    setupUI();
    loadDoctorInfo();
}

void DoctorInfoPage::setupUI()
{
    // 设置为独立的顶级窗口
    setWindowTitle("医生详细信息");
    setFixedSize(800, 600);
    setAttribute(Qt::WA_DeleteOnClose);
    
    // 设置窗口标志，使其成为独立的可移动窗口
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    
    // 设置为模态窗口，但允许用户移动
    setWindowModality(Qt::ApplicationModal);
    
    // 设置窗口图标（可选）
    // setWindowIcon(QIcon(":/icons/doctor.png"));
    
    // 居中显示窗口
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center() - rect().center());
    }
    
    // 设置窗口样式
    setStyleSheet(
        "QWidget {"
        "    background-color: #f5f5f5;"
        "}"
        "QWidget#DoctorInfoPage {"
        "    border: 2px solid #3498db;"
        "    border-radius: 10px;"
        "}"
    );
    
    // 设置对象名称以便样式表定位
    setObjectName("DoctorInfoPage");
    
    // 主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    m_mainLayout->setSpacing(10);
    
    // 标题栏和关闭按钮
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("医生详细信息");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50; margin-bottom: 5px;");
    
    m_closeButton = new QPushButton("✕");
    m_closeButton->setFixedSize(35, 35);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e74c3c;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 17px;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c0392b;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #a93226;"
        "}"
    );
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton);
    
    m_mainLayout->addLayout(headerLayout);
    
    // 分隔线
    QFrame* separatorLine = new QFrame();
    separatorLine->setFrameShape(QFrame::HLine);
    separatorLine->setFrameShadow(QFrame::Sunken);
    separatorLine->setStyleSheet("color: #bdc3c7; margin: 5px 0;");
    m_mainLayout->addWidget(separatorLine);
    
    // 滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea { "
        "    border: none; "
        "    background-color: #f5f5f5; "
        "}"
        "QScrollBar:vertical {"
        "    background-color: #ecf0f1;"
        "    width: 12px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: #bdc3c7;"
        "    border-radius: 6px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: #95a5a6;"
        "}"
    );
    
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color: #f5f5f5;");
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setSpacing(15);
    m_contentLayout->setContentsMargins(10, 10, 10, 10);
    
    // 基本信息框架
    m_basicInfoFrame = new QFrame();
    m_basicInfoFrame->setFrameStyle(QFrame::StyledPanel);
    m_basicInfoFrame->setMinimumHeight(200);
    m_basicInfoFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #ffffff;"
        "    border: 1px solid #bdc3c7;"
        "    border-radius: 8px;"
        "    margin: 5px;"
        "    box-shadow: 0 2px 4px rgba(0,0,0,0.1);"
        "}"
    );
    
    // 排班信息框架
    m_scheduleFrame = new QFrame();
    m_scheduleFrame->setFrameStyle(QFrame::StyledPanel);
    m_scheduleFrame->setMinimumHeight(150);
    m_scheduleFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #ffffff;"
        "    border: 1px solid #bdc3c7;"
        "    border-radius: 8px;"
        "    margin: 5px;"
        "    box-shadow: 0 2px 4px rgba(0,0,0,0.1);"
        "}"
    );
    
    // 统计信息框架
    m_statisticsFrame = new QFrame();
    m_statisticsFrame->setFrameStyle(QFrame::StyledPanel);
    m_statisticsFrame->setMinimumHeight(150);
    m_statisticsFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #ffffff;"
        "    border: 1px solid #bdc3c7;"
        "    border-radius: 8px;"
        "    margin: 5px;"
        "    box-shadow: 0 2px 4px rgba(0,0,0,0.1);"
        "}"
    );
    
    // 添加空的布局，等待数据加载完成后再填充
    m_contentLayout->addWidget(m_basicInfoFrame);
    m_contentLayout->addWidget(m_scheduleFrame);
    m_contentLayout->addWidget(m_statisticsFrame);
    m_contentLayout->addStretch();
    
    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);
    
    // 连接信号
    connect(m_closeButton, &QPushButton::clicked, this, &DoctorInfoPage::onCloseButtonClicked);
    
    // 不在这里显示窗口，让调用方控制显示时机
}

void DoctorInfoPage::loadDoctorInfo()
{
    qDebug() << "loadDoctorInfo called for doctor username:" << m_doctorUsername;
    
    if (!m_client) {
        QMessageBox::warning(this, "错误", "通信客户端未初始化");
        return;
    }
    
    // 使用专门的get_doctor_info接口获取医生详细信息
    QJsonObject request;
    request["action"] = "get_doctor_info";
    request["username"] = m_doctorUsername;
    
    qDebug() << "Sending get_doctor_info request for username:" << m_doctorUsername;
    
    // 连接信号处理响应
    connect(m_client, &CommunicationClient::jsonReceived, this, [this](QJsonObject response) {
        qDebug() << "=== Lambda received response ===";
        qDebug() << "Response keys:" << response.keys();
        
        QString action = response.value("action").toString();
        QString type = response.value("type").toString();
        
        qDebug() << "Action:" << action << "Type:" << type;
        
        // 处理get_doctor_info的响应
        if (action == "get_doctor_info" || type == "doctor_info_response") {
            qDebug() << "Processing doctor info response";
            
            // 断开连接避免重复处理
            disconnect(m_client, &CommunicationClient::jsonReceived, this, nullptr);
            
            if (response.value("success").toBool()) {
                QJsonObject doctorData = response.value("data").toObject();
                qDebug() << "Received doctor data:" << doctorData;
                
                if (!doctorData.isEmpty()) {
                    // 添加用户名到数据中，供显示使用
                    doctorData["username"] = m_doctorUsername;
                    displayDoctorInfo(doctorData);
                } else {
                    qDebug() << "Doctor data is empty";
                    QMessageBox::warning(this, "错误", "医生信息为空");
                }
            } else {
                QString error = response.value("error").toString();
                if (error.isEmpty()) {
                    error = response.value("message").toString();
                }
                qDebug() << "Request failed:" << error;
                QMessageBox::warning(this, "错误", "获取医生信息失败: " + error);
            }
        } else {
            qDebug() << "Ignoring unrelated response, action:" << action << "type:" << type;
        }
    });
    
    qDebug() << "Sending request...";
    m_client->sendJson(request);
}


void DoctorInfoPage::displayDoctorInfo(const QJsonObject& doctorData)
{
    qDebug() << "displayDoctorInfo called with data:" << doctorData;
    qDebug() << "Available keys:" << doctorData.keys();
    
    // 只显示基本信息，排班和统计信息暂时用默认数据
    displayBasicInfo(doctorData);
    
    // 简单的排班信息显示
    QJsonArray emptySchedules;
    displaySchedule(emptySchedules);
    
    // 简单的统计信息显示
    QJsonObject defaultStats;
    defaultStats["total_patients"] = 0;
    defaultStats["today_patients"] = 0; 
    defaultStats["month_patients"] = 0;
    defaultStats["average_rating"] = 0.0;
    displayStatistics(defaultStats);
}

void DoctorInfoPage::displayBasicInfo(const QJsonObject& doctorInfo)
{
    qDebug() << "displayBasicInfo called with data keys:" << doctorInfo.keys();
    qDebug() << "doctorInfo data:" << doctorInfo;
    
    // 清除之前的布局和占位文本
    QLayout* oldLayout = m_basicInfoFrame->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    // 创建新的布局
    QVBoxLayout* basicLayout = new QVBoxLayout(m_basicInfoFrame);
    basicLayout->setContentsMargins(15, 15, 15, 15);
    basicLayout->setSpacing(15);
    
    qDebug() << "Created basic layout for frame";
    
    // 标题
    QLabel* titleLabel = new QLabel("基本信息");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    basicLayout->addWidget(titleLabel);
    
    // 主要信息区域
    QHBoxLayout* mainInfoLayout = new QHBoxLayout();
    mainInfoLayout->setSpacing(20);
    
    // 头像区域
    QVBoxLayout* photoLayout = new QVBoxLayout();
    QLabel* photoLabel = new QLabel();
    photoLabel->setFixedSize(100, 100);
    photoLabel->setStyleSheet(
        "QLabel {"
        "    border: 2px solid #3498db;"
        "    border-radius: 10px;"
        "    background-color: #ecf0f1;"
        "    font-size: 36px;"
        "}"
    );
    photoLabel->setAlignment(Qt::AlignCenter);
    
    // 显示医生照片或默认图标
    if (doctorInfo.contains("photo") && !doctorInfo.value("photo").toString().isEmpty()) {
        QString photoBase64 = doctorInfo.value("photo").toString();
        QByteArray photoData = QByteArray::fromBase64(photoBase64.toUtf8());
        QPixmap photo;
        if (photo.loadFromData(photoData)) {
            photoLabel->setPixmap(photo.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            photoLabel->setText("👨‍⚕️");
        }
    } else {
        photoLabel->setText("👨‍⚕️");
    }
    
    photoLayout->addWidget(photoLabel);
    photoLayout->addStretch();
    
    // 详细信息区域 - 使用两列布局
    QGridLayout* detailGrid = new QGridLayout();
    detailGrid->setSpacing(10);
    detailGrid->setColumnMinimumWidth(0, 80);
    detailGrid->setColumnStretch(1, 1);
    detailGrid->setColumnMinimumWidth(2, 80);  
    detailGrid->setColumnStretch(3, 1);
    
    // 创建信息标签的函数
    auto addInfoRow = [&](int row, int col, const QString& label, const QString& value) {
        QLabel* labelWidget = new QLabel(label + ":");
        labelWidget->setStyleSheet("font-weight: bold; color: #34495e; padding: 3px;");
        
        QLabel* valueWidget = new QLabel(value.isEmpty() ? "未设置" : value);
        valueWidget->setStyleSheet(
            "color: #2c3e50; "
            "background-color: #f8f9fa; "
            "padding: 5px 8px; "
            "border-radius: 3px; "
            "border: 1px solid #e9ecef;"
        );
        valueWidget->setWordWrap(true);
        
        detailGrid->addWidget(labelWidget, row, col * 2);
        detailGrid->addWidget(valueWidget, row, col * 2 + 1);
        
        qDebug() << "Created info row:" << label << "=" << value;
    };
    
    // 添加医生信息 - 分两列显示
    int row = 0;
    addInfoRow(row, 0, "姓名", doctorInfo.value("name").toString());
    addInfoRow(row++, 1, "用户名", doctorInfo.value("username").toString());
    
    addInfoRow(row, 0, "科室", doctorInfo.value("department").toString());
    addInfoRow(row++, 1, "职称", doctorInfo.value("title").toString());
    
    addInfoRow(row, 0, "工号", doctorInfo.value("work_number").toString());
    addInfoRow(row++, 1, "挂号费", QString("¥%1").arg(doctorInfo.value("consultation_fee").toDouble(), 0, 'f', 2));
    
    addInfoRow(row, 0, "电话", doctorInfo.value("phone").toString());
    addInfoRow(row++, 1, "邮箱", doctorInfo.value("email").toString());
    
    addInfoRow(row, 0, "日患者上限", QString::number(doctorInfo.value("max_patients_per_day").toInt()));
    addInfoRow(row++, 1, "", "");  // 空白占位
    
    // 专业特长单独占一行
    QLabel* specLabel = new QLabel("专业特长:");
    specLabel->setStyleSheet("font-weight: bold; color: #34495e; padding: 3px;");
    
    QLabel* specValue = new QLabel(doctorInfo.value("specialization").toString().isEmpty() ? 
                                   "未设置" : doctorInfo.value("specialization").toString());
    specValue->setStyleSheet(
        "color: #2c3e50; "
        "background-color: #f8f9fa; "
        "padding: 8px; "
        "border-radius: 3px; "
        "border: 1px solid #e9ecef;"
    );
    specValue->setWordWrap(true);
    
    detailGrid->addWidget(specLabel, row, 0);
    detailGrid->addWidget(specValue, row, 1, 1, 3);  // 跨越3列
    
    mainInfoLayout->addLayout(photoLayout);
    mainInfoLayout->addLayout(detailGrid);
    mainInfoLayout->addStretch();
    
    basicLayout->addLayout(mainInfoLayout);
    
    qDebug() << "displayBasicInfo completed, all widgets added to layout";
}

void DoctorInfoPage::displaySchedule(const QJsonArray& schedules)
{
    // 清除之前的布局
    QLayout* oldLayout = m_scheduleFrame->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    QVBoxLayout* scheduleLayout = new QVBoxLayout(m_scheduleFrame);
    scheduleLayout->setContentsMargins(15, 15, 15, 15);
    scheduleLayout->setSpacing(10);
    
    // 标题
    QLabel* titleLabel = new QLabel("工作排班");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    scheduleLayout->addWidget(titleLabel);
    
    if (schedules.isEmpty()) {
        // 显示暂无排班信息的提示
        QFrame* infoFrame = new QFrame();
        infoFrame->setStyleSheet(
            "QFrame {"
            "    background-color: #f8f9fa;"
            "    border: 1px solid #dee2e6;"
            "    border-radius: 5px;"
            "    padding: 20px;"
            "}"
        );
        
        QVBoxLayout* infoLayout = new QVBoxLayout(infoFrame);
        
        QLabel* iconLabel = new QLabel("📅");
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("font-size: 32px; margin-bottom: 10px;");
        
        QLabel* noScheduleLabel = new QLabel("暂无排班信息");
        noScheduleLabel->setStyleSheet("color: #6c757d; font-size: 14px; font-weight: bold;");
        noScheduleLabel->setAlignment(Qt::AlignCenter);
        
        QLabel* tipLabel = new QLabel("请联系管理员设置医生的工作排班时间");
        tipLabel->setStyleSheet("color: #6c757d; font-size: 12px; margin-top: 5px;");
        tipLabel->setAlignment(Qt::AlignCenter);
        
        infoLayout->addWidget(iconLabel);
        infoLayout->addWidget(noScheduleLabel);
        infoLayout->addWidget(tipLabel);
        
        scheduleLayout->addWidget(infoFrame);
        return;
    }
    
    // 如果有排班信息，创建简化的排班表格
    QFrame* tableFrame = new QFrame();
    tableFrame->setStyleSheet(
        "QFrame {"
        "    background-color: #ffffff;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 5px;"
        "}"
    );
    
    QVBoxLayout* tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    
    // 表头
    QFrame* headerFrame = new QFrame();
    headerFrame->setStyleSheet("background-color: #e9ecef; border-bottom: 1px solid #dee2e6;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(10, 10, 10, 10);
    
    QStringList headers = {"星期", "工作时间", "最大患者数", "状态"};
    for (const QString& header : headers) {
        QLabel* headerLabel = new QLabel(header);
        headerLabel->setStyleSheet("font-weight: bold; color: #495057;");
        headerLabel->setAlignment(Qt::AlignCenter);
        headerLayout->addWidget(headerLabel);
    }
    
    tableLayout->addWidget(headerFrame);
    
    // 排班数据行
    for (int i = 0; i < schedules.size(); ++i) {
        QJsonObject schedule = schedules[i].toObject();
        
        QFrame* rowFrame = new QFrame();
        rowFrame->setStyleSheet(
            QString("background-color: %1; border-bottom: 1px solid #dee2e6;")
            .arg(i % 2 == 0 ? "#ffffff" : "#f8f9fa")
        );
        
        QHBoxLayout* rowLayout = new QHBoxLayout(rowFrame);
        rowLayout->setContentsMargins(10, 8, 10, 8);
        
        QString dayName = schedule.value("day_name").toString();
        QString startTime = schedule.value("start_time").toString();
        QString endTime = schedule.value("end_time").toString();
        int maxAppointments = schedule.value("max_appointments").toInt();
        bool isActive = schedule.value("is_active").toBool();
        
        QString timeRange = startTime + " - " + endTime;
        QString status = isActive ? "正常上班" : "休息";
        QString statusColor = isActive ? "#28a745" : "#dc3545";
        
        QStringList rowData = {dayName, timeRange, QString::number(maxAppointments), status};
        QStringList colors = {"#212529", "#212529", "#212529", statusColor};
        
        for (int j = 0; j < rowData.size(); ++j) {
            QLabel* cell = new QLabel(rowData[j]);
            cell->setStyleSheet(QString("color: %1;").arg(colors[j]));
            cell->setAlignment(Qt::AlignCenter);
            rowLayout->addWidget(cell);
        }
        
        tableLayout->addWidget(rowFrame);
    }
    
    scheduleLayout->addWidget(tableFrame);
}

void DoctorInfoPage::displayStatistics(const QJsonObject& statistics)
{
    qDebug() << "displayStatistics called with data:" << statistics;
    
    // 清除之前的布局
    QLayout* oldLayout = m_statisticsFrame->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    QVBoxLayout* statsLayout = new QVBoxLayout(m_statisticsFrame);
    statsLayout->setContentsMargins(15, 15, 15, 15);
    statsLayout->setSpacing(15);
    
    // 标题
    QLabel* titleLabel = new QLabel("统计信息");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    statsLayout->addWidget(titleLabel);
    
    // 统计数据网格 - 2x2布局
    QGridLayout* statsGrid = new QGridLayout();
    statsGrid->setSpacing(10);
    
    auto createStatItem = [&](int row, int col, const QString& label, const QString& value, const QString& color) {
        QFrame* itemFrame = new QFrame();
        itemFrame->setStyleSheet(
            "QFrame {"
            "    background-color: #ffffff;"
            "    border: 2px solid #e0e0e0;"
            "    border-radius: 6px;"
            "    padding: 10px;"
            "}"
        );
        itemFrame->setMinimumHeight(80);
        
        QVBoxLayout* itemLayout = new QVBoxLayout(itemFrame);
        itemLayout->setAlignment(Qt::AlignCenter);
        itemLayout->setSpacing(5);
        
        QLabel* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;").arg(color));
        valueLabel->setAlignment(Qt::AlignCenter);
        
        QLabel* labelWidget = new QLabel(label);
        labelWidget->setStyleSheet("font-size: 12px; color: #7f8c8d;");
        labelWidget->setAlignment(Qt::AlignCenter);
        
        itemLayout->addWidget(valueLabel);
        itemLayout->addWidget(labelWidget);
        
        statsGrid->addWidget(itemFrame, row, col);
        qDebug() << "Created stat item:" << label << "=" << value;
    };
    
    // 显示统计数据 - 如果没有数据则显示默认值0
    createStatItem(0, 0, "总患者数", QString::number(statistics.value("total_patients").toInt(0)), "#3498db");
    createStatItem(0, 1, "今日患者数", QString::number(statistics.value("today_patients").toInt(0)), "#2ecc71");
    createStatItem(1, 0, "本月患者数", QString::number(statistics.value("month_patients").toInt(0)), "#f39c12");
    createStatItem(1, 1, "平均评分", 
                   QString::number(statistics.value("average_rating").toDouble(0.0), 'f', 1) + "★", "#e74c3c");
    
    statsLayout->addLayout(statsGrid);
    
    // 添加说明文字
    QLabel* noteLabel = new QLabel("注：统计数据暂未实现，显示为默认值");
    noteLabel->setStyleSheet("color: #95a5a6; font-size: 11px; font-style: italic; margin-top: 5px;");
    noteLabel->setAlignment(Qt::AlignCenter);
    statsLayout->addWidget(noteLabel);
}

void DoctorInfoPage::onCloseButtonClicked()
{
    emit closeRequested();
    close();
}
