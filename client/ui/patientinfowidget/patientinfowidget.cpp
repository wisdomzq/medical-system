#include "patientinfowidget.h"
#include "core/network/protocol.h"
#include "core/network/communicationclient.h"
#include "appointmentpage.h"
#include "profilepage.h"
#include "casepage.h"
#include "placeholderpages.h"
#include "hospitalpage.h"
#include "medicationpage.h"
#include "evaluatepage.h"
#include "prescriptionpage.h"
#include "advicepage.h"
#include "doctorlistpage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QTabWidget>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>
#include <QIcon>

PatientInfoWidget::PatientInfoWidget(const QString &patientName, QWidget *parent)
    : QWidget(parent), m_patientName(patientName)
{
    m_communicationClient = new CommunicationClient(this);
    m_communicationClient->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
    setWindowTitle("患者中心 - " + m_patientName);
    setMinimumSize(1000, 700);

    // 应用患者界面样式
    QFile f(":/patientinfo.qss");
    if (f.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    }

    // 左侧导航 + 右侧页面
    navList = new QListWidget(this);
    navList->setObjectName("leftNav");
    navList->setIconSize(QSize(18, 18));
    navList->setUniformItemSizes(true);
    navList->setFocusPolicy(Qt::NoFocus); // 去除获得焦点时的灰色边框

    // 右侧堆叠页面
    pages = new QStackedWidget(this);

    // 创建各个页面
    m_appointmentPage = new AppointmentPage(m_communicationClient, m_patientName, this);
    m_casePage = new CasePage(m_communicationClient, m_patientName, this);
    m_communicationPage = new CommunicationPage(m_communicationClient, m_patientName, this);
    m_profilePage = new ProfilePage(m_communicationClient, m_patientName, this);
    connect(m_profilePage, &ProfilePage::backToLogin, this, &PatientInfoWidget::forwardBackToLogin);
    m_doctorInfoPage = new DoctorListPage(m_communicationClient, m_patientName, this);
    m_advicePage = new AdvicePage(m_communicationClient, m_patientName, this);
    m_prescriptionPage = new PrescriptionPage(m_communicationClient, m_patientName, this);
    
    // 连接医嘱页面到处方页面的跳转
    connect(m_advicePage, &AdvicePage::prescriptionRequested, 
            this, &PatientInfoWidget::switchToPrescriptionTab);
    
    m_evaluatePage = new EvaluatePage(m_communicationClient, m_patientName, this);
    m_medicationSearchPage = new MedicationSearchPage(m_communicationClient, m_patientName, this);
    m_hospitalPage = new HospitalPage(m_communicationClient, m_patientName, this);

    // 添加页面到堆叠组件
    pages->addWidget(m_appointmentPage);      // 0
    pages->addWidget(m_casePage);             // 1
    pages->addWidget(m_communicationPage);    // 2
    pages->addWidget(m_profilePage);          // 3
    pages->addWidget(m_doctorInfoPage);       // 4
    pages->addWidget(m_advicePage);           // 5
    pages->addWidget(m_prescriptionPage);     // 6
    pages->addWidget(m_evaluatePage);         // 7
    pages->addWidget(m_medicationSearchPage); // 8
    pages->addWidget(m_hospitalPage);         // 9

    // 导航项与图标
    auto addNav = [&](const QString &text, const QString &iconRes){
        QListWidgetItem *it = new QListWidgetItem(QIcon(iconRes), text);
        it->setSizeHint(QSize(160, 36));
        navList->addItem(it);
    };
    addNav("我的预约", ":/icons/我的预约.svg");
    addNav("我的病例", ":/icons/我的病例.svg");
    addNav("医患沟通", ":/icons/医患沟通.svg");
    addNav("个人信息", ":/icons/基本信息.svg");
    addNav("查看医生信息", ":/icons/查看医生信息.svg");
    addNav("医嘱", ":/icons/医嘱.svg");
    addNav("处方", ":/icons/处方.svg");
    addNav("评估与充值", ":/icons/评估与充值.svg");
    addNav("药品搜索", ":/icons/药品搜索.svg");
    addNav("住院信息", ":/icons/住院信息.svg");
    addNav("退出登录", ":/icons/退出登录.svg");

    // 默认选择第一项
    navList->setCurrentRow(0);
    pages->setCurrentIndex(0);

    // 同步切换
    connect(navList, &QListWidget::currentRowChanged, this, [this](int index) {
        if (index < pages->count()) {
            pages->setCurrentIndex(index);
        }
    });
    
    // 点击"退出登录"项时触发回到登录
    connect(navList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it){
        if (!it) return;
        if (it->text() == QStringLiteral("退出登录")) emit backToLogin();
    });

    // 保证选中项文字加粗
    auto updateBold = [this]() {
        for (int i = 0; i < navList->count(); ++i) {
            auto *it = navList->item(i);
            QFont f = it->font();
            f.setBold(i == navList->currentRow());
            it->setFont(f);
            it->setData(Qt::FontRole, f);
        }
        navList->viewport()->update();
    };
    updateBold();
    connect(navList, &QListWidget::currentRowChanged, this, [updateBold](int){ updateBold(); });

    // 总布局
    auto root = new QHBoxLayout(this);
    root->addWidget(navList, 0);
    root->addWidget(pages, 1);
    setLayout(root);

    connect(m_communicationClient, &CommunicationClient::jsonReceived, this, [this](const QJsonObject &obj){
        const QString type = obj.value("type").toString();
        if (type.startsWith("doctor_") || type == "register_doctor_response" || type == "appointments_response")
            m_appointmentPage->handleResponse(obj);
        if (type.startsWith("patient_info"))
            m_profilePage->handleResponse(obj);
        if (type.startsWith("hospitalizations"))
            m_hospitalPage->handleResponse(obj);
        if (type == "medications_response")
            m_medicationSearchPage->handleResponse(obj);
        if (type.startsWith("evaluate_"))
            m_evaluatePage->handleResponse(obj);
    });
}

PatientInfoWidget::~PatientInfoWidget() = default;

void PatientInfoWidget::forwardBackToLogin() { emit backToLogin(); }

void PatientInfoWidget::switchToPrescriptionTab(int prescriptionId) {
    // 切换到处方页面 (在导航栏中是第6项，索引为6)
    navList->setCurrentRow(6);
    pages->setCurrentIndex(6);
    // TODO: 如果需要，可以通知处方页面显示特定的处方
    Q_UNUSED(prescriptionId)
}
