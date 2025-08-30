#include "doctorinfopage.h"
#include "core/network/src/client/communicationclient.h"
#include <QApplication>
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
    setWindowTitle("医生详细信息");
    setFixedSize(800, 600);
    setAttribute(Qt::WA_DeleteOnClose);
    
    // 设置窗口样式
    setStyleSheet(
        "QWidget {"
        "    background-color: #f5f5f5;"
        "}"
    );
    
    // 主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 标题栏和关闭按钮
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("医生详细信息");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    
    m_closeButton = new QPushButton("✕");
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e74c3c;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 15px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c0392b;"
        "}"
    );
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton);
    
    m_mainLayout->addLayout(headerLayout);
    
    // 滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #f5f5f5; }");
    
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
    
    // 确保窗口可见
    show();
    raise();
    activateWindow();
}

void DoctorInfoPage::loadDoctorInfo()
{
    qDebug() << "loadDoctorInfo called for doctor username:" << m_doctorUsername;
    
    if (!m_client) {
        QMessageBox::warning(this, "错误", "通信客户端未初始化");
        return;
    }
    
    // 使用和医生列表相同的action，然后筛选出我们需要的医生
    QJsonObject request;
    request["action"] = "get_all_doctors";
    
    qDebug() << "Sending get_all_doctors request";
    
    // 使用和DoctorListPage完全相同的方式
    connect(m_client, &CommunicationClient::jsonReceived, this, [this](QJsonObject response) {
        qDebug() << "=== Lambda received response ===";
        qDebug() << "Response keys:" << response.keys();
        
        QString action = response.value("action").toString();
        QString type = response.value("type").toString();
        
        qDebug() << "Action:" << action << "Type:" << type;
        
        if (action == "get_all_doctors" || type == "doctors_response") {
            qDebug() << "Processing doctors response";
            
            // 断开连接避免重复处理
            disconnect(m_client, &CommunicationClient::jsonReceived, this, nullptr);
            
            if (response.value("success").toBool()) {
                QJsonArray doctors = response.value("data").toArray();
                qDebug() << "Received" << doctors.size() << "doctors";
                
                // 查找指定的医生
                QJsonObject targetDoctor;
                for (const QJsonValue& value : doctors) {
                    QJsonObject doctor = value.toObject();
                    if (doctor.value("username").toString() == m_doctorUsername) {
                        targetDoctor = doctor;
                        break;
                    }
                }
                
                if (!targetDoctor.isEmpty()) {
                    qDebug() << "Found target doctor:" << targetDoctor;
                    displayDoctorInfo(targetDoctor);
                } else {
                    qDebug() << "Doctor not found:" << m_doctorUsername;
                    QMessageBox::warning(this, "错误", "未找到指定医生信息");
                }
            } else {
                QString error = response.value("error").toString();
                qDebug() << "Request failed:" << error;
                QMessageBox::warning(this, "错误", "获取医生信息失败: " + error);
            }
        } else {
            qDebug() << "Ignoring unrelated response";
        }
    });
    
    qDebug() << "Sending request...";
    m_client->sendJson(request);
}

void DoctorInfoPage::onJsonReceived(const QJsonObject& response)
{
    qDebug() << "=== DoctorInfoPage::onJsonReceived CALLED ===";
    qDebug() << "DoctorInfoPage received ANY response:" << response;
    
    QString action = response.value("action").toString();
    QString type = response.value("type").toString();
    
    qDebug() << "Response Action:" << action << "Type:" << type;
    
    // 更宽泛地处理医生信息相关的响应
    if (action.contains("doctorinfo") || type.contains("doctorinfo") || 
        action == "doctorinfo_get_details" || type == "doctorinfo_details_response") {
        
        qDebug() << "Processing doctor info response...";
        
        // 断开信号连接，避免重复处理
        disconnect(m_client, &CommunicationClient::jsonReceived, this, &DoctorInfoPage::onJsonReceived);
        
        if (!response.value("success").toBool()) {
            QString error = response.value("error").toString();
            qDebug() << "Doctor info request failed:" << error;
            QMessageBox::warning(this, "错误", "获取医生信息失败: " + error);
            return;
        }
        
        QJsonObject doctorData = response.value("data").toObject();
        qDebug() << "Doctor data received:" << doctorData;
        
        if (doctorData.isEmpty()) {
            qDebug() << "Doctor data is empty!";
            QMessageBox::warning(this, "错误", "医生信息为空");
            return;
        }
        
        displayDoctorInfo(doctorData);
    } else {
        qDebug() << "Ignoring non-doctor-info response, action:" << action << "type:" << type;
    }
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
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    basicLayout->addWidget(titleLabel);
    
    // 主要信息区域
    QHBoxLayout* mainInfoLayout = new QHBoxLayout();
    mainInfoLayout->setSpacing(20);
    
    // 头像区域
    QVBoxLayout* photoLayout = new QVBoxLayout();
    QLabel* photoLabel = new QLabel();
    photoLabel->setFixedSize(120, 120);
    photoLabel->setStyleSheet(
        "QLabel {"
        "    border: 2px solid #3498db;"
        "    border-radius: 10px;"
        "    background-color: #ecf0f1;"
        "    font-size: 48px;"
        "}"
    );
    photoLabel->setAlignment(Qt::AlignCenter);
    
    // 显示医生照片或默认图标
    if (doctorInfo.contains("photo") && !doctorInfo.value("photo").toString().isEmpty()) {
        QString photoBase64 = doctorInfo.value("photo").toString();
        QByteArray photoData = QByteArray::fromBase64(photoBase64.toUtf8());
        QPixmap photo;
        if (photo.loadFromData(photoData)) {
            photoLabel->setPixmap(photo.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            photoLabel->setText("👨‍⚕️");
        }
    } else {
        photoLabel->setText("👨‍⚕️");
    }
    
    photoLayout->addWidget(photoLabel);
    photoLayout->addStretch();
    
    // 详细信息区域
    QVBoxLayout* detailLayout = new QVBoxLayout();
    detailLayout->setSpacing(8);
    
    // 创建信息标签的函数
    auto createInfoLabel = [&](const QString& label, const QString& value) {
        QHBoxLayout* rowLayout = new QHBoxLayout();
        
        QLabel* labelWidget = new QLabel(label + ":");
        labelWidget->setFixedWidth(80);
        labelWidget->setStyleSheet("font-weight: bold; color: #34495e;");
        
        QLabel* valueWidget = new QLabel(value.isEmpty() ? "未设置" : value);
        valueWidget->setStyleSheet("color: #2c3e50; background-color: #f8f9fa; padding: 5px; border-radius: 3px;");
        
        rowLayout->addWidget(labelWidget);
        rowLayout->addWidget(valueWidget);
        rowLayout->addStretch();
        
        detailLayout->addLayout(rowLayout);
        qDebug() << "Created info row:" << label << "=" << value;
    };
    
    // 添加医生信息 - 适应get_all_doctors返回的字段
    createInfoLabel("姓名", doctorInfo.value("name").toString());
    createInfoLabel("用户名", doctorInfo.value("username").toString());
    createInfoLabel("科室", doctorInfo.value("department").toString());
    createInfoLabel("职称", doctorInfo.value("title").toString());
    createInfoLabel("专业", doctorInfo.value("specialization").toString());
    createInfoLabel("工号", doctorInfo.value("work_number").toString());
    createInfoLabel("挂号费", QString("¥%1").arg(doctorInfo.value("consultation_fee").toDouble(), 0, 'f', 2));
    createInfoLabel("日患者上限", QString::number(doctorInfo.value("max_patients_per_day").toInt()));
    createInfoLabel("电话", doctorInfo.value("phone").toString());
    createInfoLabel("邮箱", doctorInfo.value("email").toString());
    
    mainInfoLayout->addLayout(photoLayout);
    mainInfoLayout->addLayout(detailLayout);
    mainInfoLayout->addStretch();
    
    basicLayout->addLayout(mainInfoLayout);
    
    qDebug() << "displayBasicInfo completed, all widgets added to layout";
    qDebug() << "Frame visibility:" << m_basicInfoFrame->isVisible();
    qDebug() << "Frame size:" << m_basicInfoFrame->size();
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
    scheduleLayout->setSpacing(15);
    
    // 标题
    QLabel* titleLabel = new QLabel("工作排班");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    scheduleLayout->addWidget(titleLabel);
    
    if (schedules.isEmpty()) {
        QLabel* noScheduleLabel = new QLabel("暂无排班信息");
        noScheduleLabel->setStyleSheet("color: #7f8c8d; font-style: italic; font-size: 14px; padding: 20px;");
        scheduleLayout->addWidget(noScheduleLabel);
        return;
    }
    
    // 创建排班表格
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(2);
    
    // 表头
    QStringList headers = {"星期", "上班时间", "下班时间", "最大预约数", "状态"};
    for (int i = 0; i < headers.size(); ++i) {
        QLabel* header = new QLabel(headers[i]);
        header->setStyleSheet(
            "QLabel {"
            "    font-weight: bold;"
            "    background-color: #3498db;"
            "    color: white;"
            "    padding: 10px;"
            "    border: 1px solid #2980b9;"
            "}"
        );
        header->setAlignment(Qt::AlignCenter);
        gridLayout->addWidget(header, 0, i);
    }
    
    // 排班数据
    for (int i = 0; i < schedules.size(); ++i) {
        QJsonObject schedule = schedules[i].toObject();
        
        QString dayName = schedule.value("day_name").toString();
        QString startTime = schedule.value("start_time").toString();
        QString endTime = schedule.value("end_time").toString();
        int maxAppointments = schedule.value("max_appointments").toInt();
        bool isActive = schedule.value("is_active").toBool();
        
        QStringList rowData = {
            dayName,
            startTime,
            endTime,
            QString::number(maxAppointments),
            isActive ? "正常" : "休息"
        };
        
        for (int j = 0; j < rowData.size(); ++j) {
            QLabel* cell = new QLabel(rowData[j]);
            cell->setStyleSheet(
                QString("QLabel {"
                "    padding: 8px;"
                "    border: 1px solid #bdc3c7;"
                "    background-color: %1;"
                "}").arg(i % 2 == 0 ? "#ffffff" : "#f8f9fa")
            );
            cell->setAlignment(Qt::AlignCenter);
            gridLayout->addWidget(cell, i + 1, j);
        }
    }
    
    scheduleLayout->addLayout(gridLayout);
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
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    statsLayout->addWidget(titleLabel);
    
    // 统计数据网格
    QGridLayout* statsGrid = new QGridLayout();
    statsGrid->setSpacing(15);
    
    auto createStatItem = [&](int row, int col, const QString& label, const QString& value, const QString& color) {
        QFrame* itemFrame = new QFrame();
        itemFrame->setStyleSheet(
            "QFrame {"
            "    background-color: #ffffff;"
            "    border: 1px solid #e0e0e0;"
            "    border-radius: 8px;"
            "    padding: 15px;"
            "}"
        );
        
        QVBoxLayout* itemLayout = new QVBoxLayout(itemFrame);
        itemLayout->setAlignment(Qt::AlignCenter);
        
        QLabel* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(color));
        valueLabel->setAlignment(Qt::AlignCenter);
        
        QLabel* labelWidget = new QLabel(label);
        labelWidget->setStyleSheet("font-size: 14px; color: #7f8c8d; margin-top: 5px;");
        labelWidget->setAlignment(Qt::AlignCenter);
        
        itemLayout->addWidget(valueLabel);
        itemLayout->addWidget(labelWidget);
        
        statsGrid->addWidget(itemFrame, row, col);
        qDebug() << "Created stat item:" << label << "=" << value;
    };
    
    // 显示统计数据，如果没有数据则显示默认值
    createStatItem(0, 0, "总患者数", QString::number(statistics.value("total_patients").toInt()), "#3498db");
    createStatItem(0, 1, "今日患者数", QString::number(statistics.value("today_patients").toInt()), "#2ecc71");
    createStatItem(1, 0, "本月患者数", QString::number(statistics.value("month_patients").toInt()), "#f39c12");
    createStatItem(1, 1, "平均评分", QString::number(statistics.value("average_rating").toDouble(), 'f', 1), "#e74c3c");
    
    statsLayout->addLayout(statsGrid);
}

void DoctorInfoPage::onCloseButtonClicked()
{
    emit closeRequested();
    close();
}
