#include "doctorinfowidget.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>

// 各模块头文件
#include "attendancewidget.h"
#include "checkinwidget.h"
#include "chatroomwidget.h"
#include "profilewidget.h"
#include "appointmentswidget.h"
// #include "caseswidget.h"
// #include "diagnosiswidget.h"

DoctorInfoWidget::DoctorInfoWidget(const QString &doctorName, QWidget *parent)
    : QWidget(parent), m_doctorName(doctorName)
{
    setWindowTitle("医生工作台 - " + m_doctorName);
    setMinimumSize(1000, 700);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(new AttendanceWidget(m_doctorName, this), "考勤");
    tabWidget->addTab(new CheckInWidget(m_doctorName, this), "日常打卡");
    // 请假/销假已整合进考勤模块内部按钮切换
    tabWidget->addTab(new ChatRoomWidget(m_doctorName, this), "聊天室");
    tabWidget->addTab(new ProfileWidget(m_doctorName, this), "个人信息");
    tabWidget->addTab(new AppointmentsWidget(m_doctorName, this), "预约信息");
    // 按需求合并到预约信息详情页中，移除独立的 病例详情 与 诊断/医嘱 标签

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

DoctorInfoWidget::~DoctorInfoWidget() = default;
