#include "patientinfowidget.h"
#include "core/network/protocol.h"
#include "core/network/communicationclient.h"
#include "appointmentpage.h"
#include "profilepage.h"
#include "casepage.h"
#include "placeholderpages.h"
#include "communicationpage.h"
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
#include <QFile>
#include <QIODevice>
#include <QIcon>
#include <QLabel>
#include <QFont>

PatientInfoWidget::PatientInfoWidget(const QString &patientName, CommunicationClient* sharedClient, QWidget *parent)
    : QWidget(parent), m_patientName(patientName)
{
    m_communicationClient = sharedClient;
    Q_ASSERT(m_communicationClient);
    setWindowTitle("患者中心 - " + m_patientName);
    setMinimumSize(1000, 700);

    QFile f(":/patientinfo.qss");
    if (f.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    }

    // 创建左侧面板和品牌栏（仿照医生端）
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // 品牌栏
    QWidget* brandBar = new QWidget(leftPanel);
    brandBar->setObjectName("brandBar");
    QHBoxLayout* brandLayout = new QHBoxLayout(brandBar);
    brandLayout->setContentsMargins(12, 12, 12, 12);
    brandLayout->setSpacing(8);
    QLabel* brandIcon = new QLabel(brandBar);
    brandIcon->setAttribute(Qt::WA_TranslucentBackground, true);
    brandIcon->setStyleSheet("background: transparent;");
    brandIcon->setPixmap(QIcon(":/icons/智慧医疗系统.svg").pixmap(28, 28));
    QLabel* brandText = new QLabel(QStringLiteral("  智慧医疗系统"), brandBar);
    brandText->setObjectName("brandTitle");
    QFont brandFont = brandText->font();
    brandFont.setPointSize(16);
    brandFont.setBold(true);
    brandText->setFont(brandFont);
    brandLayout->addWidget(brandIcon);
    brandLayout->addWidget(brandText);
    brandLayout->addStretch();

    navList = new QListWidget(leftPanel);
    navList->setObjectName("leftNav");
    navList->setIconSize(QSize(30, 30));
    navList->setUniformItemSizes(true);
    navList->setFocusPolicy(Qt::NoFocus);

    leftLayout->addWidget(brandBar);
    leftLayout->addWidget(navList);

    pages = new QStackedWidget(this);

    m_appointmentPage = new AppointmentPage(m_communicationClient, m_patientName, this);
    m_casePage = new CasePage(m_communicationClient, m_patientName, this);
    m_communicationPage = new CommunicationPage(m_communicationClient, m_patientName, this);
    m_profilePage = new ProfilePage(m_communicationClient, m_patientName, this);
    connect(m_profilePage, &ProfilePage::backToLogin, this, &PatientInfoWidget::forwardBackToLogin);
    m_doctorInfoPage = new DoctorListPage(m_communicationClient, m_patientName, this);
    m_advicePage = new AdvicePage(m_communicationClient, m_patientName, this);
    m_prescriptionPage = new PrescriptionPage(m_communicationClient, m_patientName, this);
    connect(m_advicePage, &AdvicePage::prescriptionRequested, this, &PatientInfoWidget::switchToPrescriptionTab);
    m_evaluatePage = new EvaluatePage(m_communicationClient, m_patientName, this);
    m_medicationSearchPage = new MedicationSearchPage(m_communicationClient, m_patientName, this);
    m_hospitalPage = new HospitalPage(m_communicationClient, m_patientName, this);

    pages->addWidget(m_appointmentPage);
    pages->addWidget(m_casePage);
    pages->addWidget(m_communicationPage);
    pages->addWidget(m_profilePage);
    pages->addWidget(m_doctorInfoPage);
    pages->addWidget(m_advicePage);
    pages->addWidget(m_prescriptionPage);
    pages->addWidget(m_evaluatePage);
    pages->addWidget(m_medicationSearchPage);
    pages->addWidget(m_hospitalPage);

    auto addNav = [&](const QString &text, const QString &iconRes){
        QListWidgetItem *it = new QListWidgetItem(QIcon(iconRes), text);
        it->setSizeHint(QSize(200, 72));
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

    navList->setCurrentRow(0);
    pages->setCurrentIndex(0);
    connect(navList, &QListWidget::currentRowChanged, this, [this](int index) {
        if (index < pages->count()) {
            pages->setCurrentIndex(index);
        }
    });
    connect(navList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it){
        if (!it) return;
        if (it->text() == QStringLiteral("退出登录")) emit backToLogin();
    });

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

    auto root = new QHBoxLayout(this);
    root->addWidget(leftPanel, 0);
    root->addWidget(pages, 1);
    setLayout(root);
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
