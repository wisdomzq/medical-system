#include "doctorinfowidget.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>

// 各模块头文件
#include "attendancewidget.h"
#include "checkinwidget.h"
#include "leavewidget.h"
#include "cancelleavewidget.h"
#include "chatroomwidget.h"
#include "profilewidget.h"
#include "appointmentswidget.h"
#include "caseswidget.h"
#include "diagnosiswidget.h"

DoctorInfoWidget::DoctorInfoWidget(const QString &doctorName, QWidget *parent)
    : QWidget(parent), m_doctorName(doctorName)
{
    setWindowTitle("医生工作台 - " + m_doctorName);
    setMinimumSize(1000, 700);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(new AttendanceWidget(m_doctorName, this), "考勤");
    tabWidget->addTab(new CheckInWidget(m_doctorName, this), "日常打卡");
    tabWidget->addTab(new LeaveWidget(m_doctorName, this), "请假");
    tabWidget->addTab(new CancelLeaveWidget(m_doctorName, this), "销假");
    tabWidget->addTab(new ChatRoomWidget(m_doctorName, this), "聊天室");
    tabWidget->addTab(new ProfileWidget(m_doctorName, this), "个人信息");
    tabWidget->addTab(new AppointmentsWidget(m_doctorName, this), "预约信息");
    tabWidget->addTab(new CasesWidget(m_doctorName, this), "病例详情");
    tabWidget->addTab(new DiagnosisWidget(m_doctorName, this), "诊断/医嘱");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

DoctorInfoWidget::~DoctorInfoWidget() = default;
