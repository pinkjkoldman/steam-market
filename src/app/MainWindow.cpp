#include "app/MainWindow.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>

#include "ui/pages/DetailPage.h"
#include "ui/pages/InventoryAssistantPage.h"
#include "ui/pages/MarketBrowserPage.h"
#include "ui/pages/MarketOverviewPage.h"
#include "ui/pages/MarketPage.h"
#include "ui/pages/PortfolioPage.h"
#include "ui/pages/RankingPage.h"
#include "ui/pages/RulesPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/WatchlistPage.h"
#include "ui/pages/WelcomePage.h"
#include "ui/widgets/AccountCard.h"

MainWindow::MainWindow(WelcomePage *welcomePage, MarketOverviewPage *overviewPage,
                       MarketBrowserPage *browserPage, AccountCard *accountCard,
                       MarketPage *marketPage, DetailPage *detailPage,
                       WatchlistPage *watchlistPage, PortfolioPage *portfolioPage,
                       RankingPage *rankingPage, RulesPage *rulesPage,
                       InventoryAssistantPage *inventoryPage, SettingsPage *settingsPage,
                       QWidget *parent)
    : QMainWindow(parent), m_detail(detailPage), m_browser(browserPage) {
    setWindowTitle(QStringLiteral("Steam 市场工作台"));
    resize(1280, 800);
    setMinimumSize(1040, 680);
    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget#appRoot, QWidget#contentRoot { background:#0D1117; color:#E6EDF3; }"
        "QWidget { font-size:13px; }"
        "QFrame#sidebar { background:#111821; border-right:1px solid #263241; }"
        "QLabel#brandMark { background:#3478F6; color:white; border-radius:9px; font-size:16px; font-weight:800; }"
        "QLabel#brandTitle { color:#F3F7FC; font-size:16px; font-weight:700; }"
        "QLabel#brandSubtitle, QLabel#sidebarHint { color:#748196; font-size:11px; }"
        "QListWidget#navigation { background:transparent; border:none; color:#AEBACB; outline:none; }"
        "QListWidget#navigation::item { min-height:38px; padding:2px 12px; margin:2px 0; border-radius:7px; border-left:3px solid transparent; }"
        "QListWidget#navigation::item:hover { background:#182333; color:#F0F5FC; }"
        "QListWidget#navigation::item:selected { background:#1C3152; color:#86B5FF; border-left-color:#4C8DFF; font-weight:700; }"
        "QLabel#shellPageTitle { color:#F3F7FC; font-size:24px; font-weight:700; }"
        "QLabel#shellPageSubtitle { color:#8B98AA; font-size:12px; }"
        "QFrame#headerDivider { background:#222D3B; min-height:1px; max-height:1px; }"
        "QTableView,QTableWidget { background:#121A24; alternate-background-color:#151F2B; color:#DCE5F0; border:1px solid #273344; border-radius:8px; gridline-color:#223040; selection-background-color:#204576; }"
        "QHeaderView::section { background:#172230; color:#95A6BA; border:none; border-bottom:1px solid #304055; padding:9px 8px; font-weight:700; }"
        "QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox { background:#151F2B; color:#E6EDF3; border:1px solid #34445A; border-radius:6px; padding:7px 9px; }"
        "QPushButton { background:#1A2635; color:#D9E4F2; border:1px solid #35465C; border-radius:6px; padding:7px 14px; font-weight:600; }"
        "QPushButton:hover { background:#22344A; border-color:#59708D; color:white; }"
        "QPushButton#primaryButton { background:#3478F6; color:white; border-color:#3478F6; }"
        "QPushButton#linkButton,QPushButton#ghostButton { background:transparent; color:#7EB0FF; border-color:transparent; }"
        "QStatusBar { background:#111821; color:#8C99AA; border-top:1px solid #263241; font-size:11px; }"
        "QLabel#connectionPill { background:#142B24; color:#7FD5AD; border:1px solid #275440; border-radius:9px; padding:2px 8px; }"
        "QGroupBox,QTextBrowser { background:#111923; color:#DCE5F0; border:1px solid #29384B; border-radius:8px; }"));

    auto *sidebar = new QFrame(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(224);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(14, 18, 14, 14);
    side->setSpacing(8);

    auto *brandRow = new QHBoxLayout();
    auto *mark = new QLabel(QStringLiteral("SM"), sidebar);
    mark->setObjectName(QStringLiteral("brandMark"));
    mark->setFixedSize(42, 42);
    mark->setAlignment(Qt::AlignCenter);
    auto *brandText = new QVBoxLayout();
    auto *brand = new QLabel(QStringLiteral("Steam Market"), sidebar);
    brand->setObjectName(QStringLiteral("brandTitle"));
    auto *tagline = new QLabel(QStringLiteral("全市场行情工作台"), sidebar);
    tagline->setObjectName(QStringLiteral("brandSubtitle"));
    brandText->addWidget(brand);
    brandText->addWidget(tagline);
    brandRow->addWidget(mark);
    brandRow->addLayout(brandText, 1);
    side->addLayout(brandRow);

    m_pageNames = {QStringLiteral("overview"), QStringLiteral("browser"),
                   QStringLiteral("market"), QStringLiteral("watchlist"),
                   QStringLiteral("portfolio"), QStringLiteral("ranking"),
                   QStringLiteral("rules"), QStringLiteral("inventory"),
                   QStringLiteral("settings")};
    m_pageTitles = {QStringLiteral("市场概览"), QStringLiteral("物品浏览"),
                    QStringLiteral("价格分析"), QStringLiteral("我的关注"),
                    QStringLiteral("持仓与模拟"), QStringLiteral("市场排行"),
                    QStringLiteral("交易规则"), QStringLiteral("库存助手"),
                    QStringLiteral("偏好设置"), QStringLiteral("物品详情")};
    const QStringList labels = {QStringLiteral("市场概览"), QStringLiteral("物品浏览"),
                                QStringLiteral("价格分析"), QStringLiteral("我的关注"),
                                QStringLiteral("持仓与模拟"), QStringLiteral("市场排行"),
                                QStringLiteral("交易规则"), QStringLiteral("库存助手"),
                                QStringLiteral("偏好设置")};
    m_nav = new QListWidget(sidebar);
    m_nav->setObjectName(QStringLiteral("navigation"));
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    for (int i = 0; i < labels.size(); ++i) {
        auto *item = new QListWidgetItem(labels.at(i), m_nav);
        item->setSizeHint(QSize(190, 42));
        if (i < 9) item->setToolTip(QStringLiteral("%1 · Alt+%2").arg(labels.at(i)).arg(i + 1));
    }
    side->addWidget(m_nav, 1);
    side->addWidget(accountCard);
    auto *hint = new QLabel(QStringLiteral("公开行情无需登录\n数据来自 Steam 官方市场公开页面"), sidebar);
    hint->setObjectName(QStringLiteral("sidebarHint"));
    hint->setWordWrap(true);
    side->addWidget(hint);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(overviewPage);
    m_stack->addWidget(browserPage);
    m_stack->addWidget(marketPage);
    m_stack->addWidget(watchlistPage);
    m_stack->addWidget(portfolioPage);
    m_stack->addWidget(rankingPage);
    m_stack->addWidget(rulesPage);
    m_stack->addWidget(inventoryPage);
    m_stack->addWidget(settingsPage);
    m_stack->addWidget(detailPage);
    m_detailIndex = 9;

    auto *content = new QWidget(this);
    content->setObjectName(QStringLiteral("contentRoot"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 18, 24, 14);
    contentLayout->setSpacing(10);
    m_pageTitle = new QLabel(content);
    m_pageTitle->setObjectName(QStringLiteral("shellPageTitle"));
    m_pageSubtitle = new QLabel(content);
    m_pageSubtitle->setObjectName(QStringLiteral("shellPageSubtitle"));
    contentLayout->addWidget(m_pageTitle);
    contentLayout->addWidget(m_pageSubtitle);
    auto *divider = new QFrame(content);
    divider->setObjectName(QStringLiteral("headerDivider"));
    contentLayout->addWidget(divider);
    contentLayout->addWidget(m_stack, 1);

    auto *workbench = new QWidget(this);
    workbench->setObjectName(QStringLiteral("appRoot"));
    auto *workbenchLayout = new QHBoxLayout(workbench);
    workbenchLayout->setContentsMargins(0, 0, 0, 0);
    workbenchLayout->setSpacing(0);
    workbenchLayout->addWidget(sidebar);
    workbenchLayout->addWidget(content, 1);

    m_experience = new QStackedWidget(this);
    m_experience->addWidget(welcomePage);
    m_experience->addWidget(workbench);
    setCentralWidget(m_experience);

    m_connectionLabel = new QLabel(QStringLiteral("● Steam 数据源就绪"), this);
    m_connectionLabel->setObjectName(QStringLiteral("connectionPill"));
    m_statusLabel = new QLabel(QStringLiteral("等待首次同步"), this);
    statusBar()->addWidget(m_connectionLabel);
    statusBar()->addWidget(m_statusLabel, 1);

    connect(m_nav, &QListWidget::currentRowChanged, this,
            [this](int index) { if (index >= 0) activatePage(index); });
    connect(marketPage, &MarketPage::itemSelected, this,
            [this](const MarketItem &item) { showDetail(item.marketHashName, item.appid); });
    connect(overviewPage, &MarketOverviewPage::itemActivated, this,
            [this](int appid, const QString &name) { showDetail(name, appid); });
    connect(browserPage, &MarketBrowserPage::itemActivated, this,
            [this](int appid, const QString &name) { showDetail(name, appid); });
    connect(overviewPage, &MarketOverviewPage::browseAllRequested, this,
            [this](const QString &query, int appid) {
                m_browser->setInitialQuery(query, appid);
                navigateTo(QStringLiteral("browser"));
            });
    connect(watchlistPage, &WatchlistPage::itemSelected, this, &MainWindow::showDetail);
    connect(portfolioPage, &PortfolioPage::itemSelected, this, &MainWindow::showDetail);
    connect(rankingPage, &RankingPage::itemSelected, this, &MainWindow::showDetail);
    connect(detailPage, &DetailPage::backRequested, this,
            [this]() { activatePage(m_detailSourceIndex); });
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this]() {
        if (m_stack->currentIndex() == m_detailIndex) activatePage(m_detailSourceIndex);
    });
    for (int i = 0; i < labels.size(); ++i) {
        auto *shortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+%1").arg(i + 1)), this);
        connect(shortcut, &QShortcut::activated, this, [this, i]() { m_nav->setCurrentRow(i); });
    }
    m_nav->setCurrentRow(0);
}

void MainWindow::setWelcomeVisible(bool visible) {
    if (m_experience) m_experience->setCurrentIndex(visible ? 0 : 1);
    statusBar()->setVisible(!visible);
}

bool MainWindow::isWelcomeVisible() const {
    return m_experience && m_experience->currentIndex() == 0;
}

void MainWindow::setStatus(const QString &network, const QString &sync) {
    m_connectionLabel->setText(QStringLiteral("● %1").arg(network));
    m_statusLabel->setText(QStringLiteral("上次同步：%1").arg(sync));
}

void MainWindow::navigateTo(const QString &pageName) {
    const int index = m_pageNames.indexOf(pageName);
    if (index >= 0) {
        setWelcomeVisible(false);
        m_nav->setCurrentRow(index);
        activatePage(index);
    }
}

void MainWindow::showDetail(const QString &marketHashName, int appid) {
    if (marketHashName.isEmpty()) return;
    m_detail->showItem(marketHashName, appid);
    if (m_stack->currentIndex() != m_detailIndex && m_stack->currentIndex() >= 0)
        m_detailSourceIndex = m_stack->currentIndex();
    setWelcomeVisible(false);
    m_stack->setCurrentIndex(m_detailIndex);
    updatePageHeader(m_detailIndex);
}

void MainWindow::activatePage(int index) {
    if (index < 0 || index >= m_detailIndex) return;
    m_stack->setCurrentIndex(index);
    updatePageHeader(index);
}

void MainWindow::updatePageHeader(int index) {
    if (index < 0 || index >= m_pageTitles.size()) return;
    m_pageTitle->setText(m_pageTitles.at(index));
    if (index == m_detailIndex) {
        m_pageSubtitle->setText(QStringLiteral("来源：%1 · 按 Esc 返回")
                                    .arg(m_pageTitles.value(m_detailSourceIndex)));
        return;
    }
    const QStringList subtitles = {
        QStringLiteral("Steam 全市场规模、热门挂单与我的关注摘要"),
        QStringLiteral("搜索 Steam 当前可检索市场；每次仅按需加载 10 条"),
        QStringLiteral("查看单品价格、成交记录与盘口参考"),
        QStringLiteral("集中跟踪关注物品并管理提醒"),
        QStringLiteral("记录成本、模拟交易并观察组合盈亏"),
        QStringLiteral("按成交量和涨跌发现市场物品"),
        QStringLiteral("查询平台规则并核算费用"),
        QStringLiteral("登录后查看本人库存，或输入公开 Steam ID"),
        QStringLiteral("管理启动身份、默认游戏、显示与本地数据")};
    m_pageSubtitle->setText(subtitles.value(index));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_closeToTray) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}