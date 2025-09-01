#include "doctorinfowidget.h"
#include <QFile>
#include <QHBoxLayout>
#include <QIODevice>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

// 各模块头文件
#include "appointmentswidget.h"
#include "attendancewidget.h"
#include "chatroomwidget.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "datachartwidget.h"
#include "profilewidget.h"
#include "remotedatawidget.h"
// #include "caseswidget.h"
// #include "diagnosiswidget.h"

DoctorInfoWidget::DoctorInfoWidget(const QString& doctorName, CommunicationClient* sharedClient, QWidget* parent)
    : QWidget(parent)
    , m_doctorName(doctorName)
{
    // 使用外部共享客户端
    m_sharedClient = sharedClient;
    Q_ASSERT(m_sharedClient);
    initUi();
}

DoctorInfoWidget::~DoctorInfoWidget() = default;

void DoctorInfoWidget::initUi()
{
    setWindowTitle("医生工作台 - " + m_doctorName);
    setMinimumSize(1000, 700);

    QFile f(":/doctorinfo.qss");
    if (f.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    }

    QWidget* leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

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

    auto attendance = new AttendanceWidget(m_doctorName, m_sharedClient, this);
    auto chat = new ChatRoomWidget(m_doctorName, m_sharedClient, this);
    auto profile = new ProfileWidget(m_doctorName, m_sharedClient, this);
    auto appts = new AppointmentsWidget(m_doctorName, m_sharedClient, this);
    auto charts = new DataChartWidget(m_doctorName, this);
    auto remote = new RemoteDataWidget(m_doctorName, this);

    pages->addWidget(attendance);
    pages->addWidget(chat);
    pages->addWidget(profile);
    pages->addWidget(appts);
    pages->addWidget(charts);
    pages->addWidget(remote);

    auto addNav = [&](const QString& text, const QString& iconRes) {
        QListWidgetItem* it = new QListWidgetItem(QIcon(iconRes), text);
        it->setSizeHint(QSize(200, 72));
        navList->addItem(it);
    };
    addNav("考勤", ":/icons/考勤.svg");
    addNav("聊天室", ":/icons/聊天室.svg");
    addNav("个人信息", ":/icons/基本信息.svg");
    addNav("预约信息", ":/icons/预约信息.svg");
    addNav("数据图表", ":/icons/数据图表.svg");
    addNav("远程数据采集", ":/icons/远程数据采集.svg");
    addNav("退出登录", ":/icons/退出登录.svg");

    navList->setCurrentRow(0);
    pages->setCurrentIndex(0);

    connect(navList, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    connect(navList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        if (!it) return;
        if (it->text() == QStringLiteral("退出登录")) emit backToLogin();
    });

    auto updateBold = [this]() {
        for (int i = 0; i < navList->count(); ++i) {
            auto* it = navList->item(i);
            QFont f = it->font();
            f.setBold(i == navList->currentRow());
            it->setFont(f);
            it->setData(Qt::FontRole, f);
        }
        navList->viewport()->update();
    };
    updateBold();
    connect(navList, &QListWidget::currentRowChanged, this, [updateBold](int) { updateBold(); });

    auto root = new QHBoxLayout(this);
    root->addWidget(leftPanel, 0);
    root->addWidget(pages, 1);
    setLayout(root);

    
}

// 预约与医生个人信息均由各自 Service 监听网络并驱动 UI，此处无需集中分发
