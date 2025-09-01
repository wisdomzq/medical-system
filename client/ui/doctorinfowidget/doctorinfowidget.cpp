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

DoctorInfoWidget::DoctorInfoWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent)
    , m_doctorName(doctorName)
{
    setWindowTitle("医生工作台 - " + m_doctorName);
    setMinimumSize(1000, 700);

    // 创建共享的通信客户端
    m_sharedClient = new CommunicationClient(this);
    m_sharedClient->connectToServer("127.0.0.1", Protocol::SERVER_PORT);

    // 仅对医生界面应用样式
    QFile f(":/doctorinfo.qss");
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

    // 构建页面 - 传递共享的通信客户端
    auto attendance = new AttendanceWidget(m_doctorName, m_sharedClient, this);
    auto chat = new ChatRoomWidget(m_doctorName, m_sharedClient, this);
    auto profile = new ProfileWidget(m_doctorName, m_sharedClient, this);
    auto appts = new AppointmentsWidget(m_doctorName, m_sharedClient, this); // 使用新的构造函数
    auto charts = new DataChartWidget(m_doctorName, this);
    auto remote = new RemoteDataWidget(m_doctorName, this);

    // 添加至堆叠
    pages->addWidget(attendance); // 0
    pages->addWidget(chat); // 1
    pages->addWidget(profile); // 2
    pages->addWidget(appts); // 3
    pages->addWidget(charts); // 4
    pages->addWidget(remote); // 5

    // 导航项与图标（当前使用资源中的占位图标，可后续替换为具体图标）
    auto addNav = [&](const QString& text, const QString& iconRes) {
        QListWidgetItem* it = new QListWidgetItem(QIcon(iconRes), text);
        it->setSizeHint(QSize(160, 36));
        navList->addItem(it);
    };
    addNav("考勤", ":/icons/考勤.svg");
    addNav("聊天室", ":/icons/聊天室.svg");
    addNav("个人信息", ":/icons/基本信息.svg");
    addNav("预约信息", ":/icons/预约信息.svg");
    addNav("数据图表", ":/icons/数据图表.svg");
    addNav("远程数据采集", ":/icons/远程数据采集.svg");
    addNav("退出登录", ":/icons/退出登录.svg");

    // 默认选择第一项
    navList->setCurrentRow(0);
    pages->setCurrentIndex(0);

    // 同步切换
    connect(navList, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    // 点击“退出登录”项时触发回到登录
    connect(navList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        if (!it)
            return;
        if (it->text() == QStringLiteral("退出登录"))
            emit backToLogin();
    });

    // 保证选中项文字加粗（某些平台 QSS 对 item 的 font-weight 不生效，使用代码加粗）
    auto updateBold = [this]() {
        for (int i = 0; i < navList->count(); ++i) {
            auto* it = navList->item(i);
            QFont f = it->font();
            f.setBold(i == navList->currentRow());
            it->setFont(f);
            it->setData(Qt::FontRole, f); // 通过模型数据设置，兼容某些样式/平台
        }
        navList->viewport()->update();
    };
    updateBold();
    connect(navList, &QListWidget::currentRowChanged, this, [updateBold](int) { updateBold(); });

    // 总布局
    auto root = new QHBoxLayout(this);
    root->addWidget(navList, 0);
    root->addWidget(pages, 1);
    setLayout(root);
}

DoctorInfoWidget::DoctorInfoWidget(const QString& doctorName, CommunicationClient* sharedClient, QWidget* parent)
    : QWidget(parent)
    , m_doctorName(doctorName)
{
    setWindowTitle("医生工作台 - " + m_doctorName);
    setMinimumSize(1000, 700);

    // 使用外部共享客户端
    m_sharedClient = sharedClient;
    Q_ASSERT(m_sharedClient);

    // 仅对医生界面应用样式
    QFile f(":/doctorinfo.qss");
    if (f.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
    }

    // 左侧导航 + 右侧页面
    navList = new QListWidget(this);
    navList->setObjectName("leftNav");
    navList->setIconSize(QSize(18, 18));
    navList->setUniformItemSizes(true);
    navList->setFocusPolicy(Qt::NoFocus);

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
        it->setSizeHint(QSize(160, 36));
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
    root->addWidget(navList, 0);
    root->addWidget(pages, 1);
    setLayout(root);
}

DoctorInfoWidget::~DoctorInfoWidget() = default;

// 预约与医生个人信息均由各自 Service 监听网络并驱动 UI，此处无需集中分发
