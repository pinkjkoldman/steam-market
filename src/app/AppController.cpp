#include "app/AppController.h"

#include <QApplication>
#include <QChartView>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkProxy>
#include <QPixmap>
#include <QPushButton>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QUrl>
#include <QtDebug>
#include <memory>

#include "app/MainWindow.h"
#include "core/services/AlertService.h"
#include "core/services/KlineService.h"
#include "core/services/FullMarketService.h"
#include "core/services/InventoryService.h"
#include "core/services/MultiSellHandoffService.h"
#include "core/services/PricingDraftService.h"
#include "core/services/SteamSessionService.h"
#include "core/services/MarketService.h"
#include "core/services/NotificationService.h"
#include "core/services/OrderbookService.h"
#include "core/services/PortfolioService.h"
#include "core/services/PriceSourceService.h"
#include "core/services/RankingService.h"
#include "core/services/SettingsService.h"
#include "core/services/TradeSimulationService.h"
#include "core/services/TradingRulesService.h"
#include "core/services/WatchlistService.h"
#include "data/DatabaseManager.h"
#include "data/repositories/AlertRepository.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/InventoryRepository.h"
#include "data/repositories/OrderbookRepository.h"
#include "data/repositories/MarketCatalogRepository.h"
#include "data/repositories/PortfolioRepository.h"
#include "data/repositories/PriceRepository.h"
#include "data/repositories/SettingsRepository.h"
#include "data/repositories/TradeRepository.h"
#include "data/repositories/WatchlistRepository.h"
#include "network/SteamMarketClient.h"
#include "network/SteamMarketCatalogClient.h"
#include "network/SteamInventoryClient.h"
#include "network/WebView2SessionHost.h"
#include "ui/TrayManager.h"
#include "ui/pages/DetailPage.h"
#include "ui/pages/MarketBrowserPage.h"
#include "ui/pages/MarketOverviewPage.h"
#include "ui/pages/WelcomePage.h"
#include "ui/pages/MarketPage.h"
#include "ui/pages/PortfolioPage.h"
#include "ui/pages/RankingPage.h"
#include "ui/pages/RulesPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/InventoryAssistantPage.h"
#include "ui/pages/WatchlistPage.h"
#include "ui/widgets/AccountCard.h"
#include "ui/widgets/PriceChart.h"
#include "ui/widgets/WorkbenchModels.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"
#include "utils/ProxyConfig.h"
#include "utils/ThemeProvider.h"

namespace {
QString dataDirPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}
}  // namespace

AppController::AppController(QObject *parent) : QObject(parent) {}

AppController::~AppController() {
    releaseRepositories();
    delete m_dbManager;
}

void AppController::releaseRepositories() {
    // 非 QObject 成员显式释放（数据库连接必须先于仓储删除）。
    delete m_catalogRepo;
    delete m_handoff;
    delete m_pricingDraft;
    delete m_inventoryRepo;
    delete m_settingsRepo;
    delete m_portfolioRepo;
    delete m_alertRepo;
    delete m_watchlistRepo;
    delete m_tradeRepo;
    delete m_orderbookRepo;
    delete m_prices;
    delete m_items;
    m_handoff = nullptr;
    m_pricingDraft = nullptr;
    m_catalogRepo = nullptr;
    m_inventoryRepo = nullptr;
    m_settingsRepo = nullptr;
    m_portfolioRepo = nullptr;
    m_alertRepo = nullptr;
    m_watchlistRepo = nullptr;
    m_tradeRepo = nullptr;
    m_orderbookRepo = nullptr;
    m_prices = nullptr;
    m_items = nullptr;
    m_db = QSqlDatabase();
}

bool AppController::initialize() {
    const QString baseDir = dataDirPath();
    QDir().mkpath(baseDir);
    m_dbManager = new DatabaseManager(baseDir + QStringLiteral("/steam_market.db"));
    if (!m_dbManager->open()) {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"),
                              QStringLiteral("无法打开本地数据库，请检查 %1 目录权限")
                                  .arg(baseDir));
        return false;
    }
    m_dbManager->runRetentionPolicy();
    m_db = m_dbManager->database();
    buildServices(m_db);
    buildUi();
    wireSignals();
    startRefreshTimer();
    applySettings();
    m_watchlist->refreshAll();
    return true;
}

void AppController::show() {
    if (m_mainWindow) {
        m_mainWindow->show();
        m_tray->showIcon();
    }
}

void AppController::bringToFront() {
    if (!m_mainWindow) return;
    m_mainWindow->show();
    m_mainWindow->setWindowState(m_mainWindow->windowState() & ~Qt::WindowMinimized);
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}

void AppController::buildServices(QSqlDatabase &db) {
    m_items = new ItemRepository(db);
    m_catalogRepo = new MarketCatalogRepository(db);
    m_inventoryRepo = new InventoryRepository(db);
    m_prices = new PriceRepository(db);
    m_watchlistRepo = new WatchlistRepository(db);
    m_alertRepo = new AlertRepository(db);
    m_catalogClient = new SteamMarketCatalogClient(this);
    m_fullMarket = new FullMarketService(m_catalogRepo, m_catalogClient, this);
    m_portfolioRepo = new PortfolioRepository(db);
    m_settingsRepo = new SettingsRepository(db);
    m_orderbookRepo = new OrderbookRepository(db);
    m_tradeRepo = new TradeRepository(db);
    m_nam = new QNetworkAccessManager(this);
    m_jar = new QNetworkCookieJar(m_nam);
    m_nam->setCookieJar(m_jar);
    m_client = new SteamMarketClient(m_nam, this);
    m_market = new MarketService(m_client, m_items, m_prices, this);
    m_watchlist = new WatchlistService(m_watchlistRepo, m_prices, m_items, m_client, this);
    m_alerts = new AlertService(m_alertRepo, m_prices, this);
    m_portfolio = new PortfolioService(m_portfolioRepo, m_prices, m_items, this);
    m_portfolio->setClient(m_client);
    m_sources = new PriceSourceService(db, m_prices, this);
    m_settings = new SettingsService(m_settingsRepo, this);
    m_rules = new TradingRulesService(m_settings, this);
    m_kline = new KlineService(m_prices, this);
    m_orderbook = new OrderbookService(m_client, m_orderbookRepo, this);
    m_ranking = new RankingService(m_watchlistRepo, m_prices, m_items, this);
    m_trades = new TradeSimulationService(m_tradeRepo, m_items, m_rules, m_portfolio, this);
    m_inventoryClient = new SteamInventoryClient(m_nam, this);
    m_webSessionHost = new WebView2SessionHost(this);
    m_steamSession = new SteamSessionService(m_webSessionHost, m_nam, this);
    m_inventoryService = new InventoryService(m_inventoryRepo, m_inventoryClient, this);
    m_pricingDraft = new PricingDraftService();
    m_handoff = new MultiSellHandoffService();
    m_notifications = new NotificationService(this);
    m_servicesBuilt = true;
}

void AppController::buildUi() {
    m_welcomePage = new WelcomePage();
    m_overviewPage = new MarketOverviewPage();
    m_browserPage = new MarketBrowserPage();
    auto *accountCard = new AccountCard();
    auto *marketPage = new MarketPage(m_market);
    m_detailPage = new DetailPage(m_market, m_watchlist, m_alerts, m_sources, m_kline,
                                  m_orderbook);
    auto *watchlistPage = new WatchlistPage(m_watchlist, m_alerts);
    auto *portfolioPage = new PortfolioPage(m_portfolio, m_trades);
    auto *rankingPage = new RankingPage(m_ranking);
    auto *rulesPage = new RulesPage(m_rules);
    auto *inventoryPage = new InventoryAssistantPage(
        m_steamSession, m_inventoryService, m_pricingDraft, m_handoff);
    auto *settingsPage = new SettingsPage(m_settings);
    m_mainWindow = new MainWindow(m_welcomePage, m_overviewPage, m_browserPage, accountCard,
                                  marketPage, m_detailPage, watchlistPage, portfolioPage,
                                  rankingPage, rulesPage, inventoryPage, settingsPage);
    m_tray = new TrayManager(this);
}
void AppController::wireSignals() {
    auto lastCatalogQuery = std::make_shared<MarketCatalogQuery>();
    auto catalogQuery = [this, lastCatalogQuery](const QString &query, int appid,
                                                 int offset, int limit,
                                                 const QString &sort,
                                                 const QString &currency) {
        MarketCatalogQuery request;
        request.query = query;
        if (appid > 0) request.appid = appid;
        request.offset = offset;
        request.limit = qBound(1, limit, 10);
        request.currency = currency;
        if (sort == QStringLiteral("price_asc")) request.sort = CatalogSort::kPriceAsc;
        else if (sort == QStringLiteral("price_desc")) request.sort = CatalogSort::kPriceDesc;
        else if (sort == QStringLiteral("name_asc")) request.sort = CatalogSort::kNameAsc;
        *lastCatalogQuery = request;
        m_fullMarket->requestPage(request);
    };
    connect(m_overviewPage, &MarketOverviewPage::popularPageRequested, this, catalogQuery);
    connect(m_browserPage, &MarketBrowserPage::catalogRequested, this, catalogQuery);
    connect(m_overviewPage, &MarketOverviewPage::retryRequested, this,
            [this, lastCatalogQuery]() { m_fullMarket->requestPage(*lastCatalogQuery, true); });
    connect(m_browserPage, &MarketBrowserPage::retryRequested, this,
            [this, lastCatalogQuery]() { m_fullMarket->requestPage(*lastCatalogQuery, true); });
    connect(m_fullMarket, &FullMarketService::requestStateChanged, this, [this](bool busy) {
        MarketUiStatus status;
        status.state = busy ? MarketViewState::Loading : MarketViewState::Ready;
        status.message = busy ? QStringLiteral("正在读取 Steam 官方市场公开页面…")
                              : QStringLiteral("Steam 市场数据已就绪");
        m_overviewPage->setStatus(status);
        m_browserPage->setStatus(status);
    });    connect(m_fullMarket, &FullMarketService::pageReady, this,
            [this](const MarketCatalogPage &page) {
                MarketCatalogPageView view;
                view.offset = page.offset;
                view.pageSize = page.pageSize;
                view.totalCount = page.totalCount;
                view.fetchedAt = page.fetchedAt;
                view.origin = page.origin == DataOrigin::kSteamCached
                                  ? MarketDataOrigin::SteamCached
                                  : MarketDataOrigin::SteamLive;
                view.stale = page.stale;
                view.sourceLabel = page.sourceLabel;
                for (const auto &item : page.items) {
                    view.items.append({item.appid, item.marketHashName, item.localizedName,
                                       item.typeText, item.iconUrl, item.lowestSellMinor,
                                       item.sellListings});
                }
                if (page.query.offset == 0 && page.query.sort == CatalogSort::kPopular
                    && page.query.query.trimmed().isEmpty()) {
                    m_overviewPage->setPopularPage(view);
                }
                m_browserPage->setPage(view);
                QVector<MarketScopeSnapshotView> scopes;
                for (const auto &scope : m_fullMarket->scopeSnapshots(
                         {QStringLiteral("all"), QStringLiteral("appid:730"),
                          QStringLiteral("appid:570"), QStringLiteral("appid:753")})) {
                    scopes.append({scope.scopeKey, scope.totalCount, scope.fetchedAt,
                                   scope.origin == DataOrigin::kSteamCached
                                       ? MarketDataOrigin::SteamCached
                                       : MarketDataOrigin::SteamLive});
                }
                m_overviewPage->setScopeSnapshots(scopes);
            });
    connect(m_fullMarket, &FullMarketService::pageFailed, this,
            [this](const MarketCatalogError &error) {
                MarketUiStatus status;
                status.message = QStringLiteral("Steam 市场暂不可用，请稍后重试");
                status.retryAfterMs = error.retryAfterMs.value_or(0);
                switch (error.code) {
                case MarketCatalogErrorCode::kRateLimited:
                    status.state = MarketViewState::RateLimited; break;
                case MarketCatalogErrorCode::kSchemaChanged:
                    status.state = MarketViewState::SchemaChanged; break;
                case MarketCatalogErrorCode::kOfflineNoCache:
                    status.state = MarketViewState::OfflineNoCache; break;
                default:
                    status.state = MarketViewState::Error; break;
                }
                m_overviewPage->setStatus(status);
                m_browserPage->setStatus(status);
            });
    connect(m_overviewPage, &MarketOverviewPage::scopeSummaryRequested, this, [this]() {
        QVector<MarketScopeSnapshotView> scopes;
        for (const auto &scope : m_fullMarket->scopeSnapshots()) {
            scopes.append({scope.scopeKey, scope.totalCount, scope.fetchedAt,
                           scope.origin == DataOrigin::kSteamCached
                               ? MarketDataOrigin::SteamCached
                               : MarketDataOrigin::SteamLive});
        }
        m_overviewPage->setScopeSnapshots(scopes);
    });
    auto openOfficialMarket = []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://steamcommunity.com/market/")));
    };
    connect(m_overviewPage, &MarketOverviewPage::openOfficialMarketRequested, this,
            openOfficialMarket);
    connect(m_browserPage, &MarketBrowserPage::openOfficialMarketRequested, this,
            openOfficialMarket);
    connect(m_overviewPage, &MarketOverviewPage::manageWatchlistRequested, this,
            [this]() { m_mainWindow->navigateTo(QStringLiteral("watchlist")); });
    connect(m_overviewPage, &MarketOverviewPage::manageAlertsRequested, this,
            [this]() { m_mainWindow->navigateTo(QStringLiteral("watchlist")); });
    auto inspectedName = std::make_shared<QString>();
    auto inspectItem = [this, inspectedName](int appid, const QString &marketHashName) {
        *inspectedName = marketHashName;
        MarketInspectionView loading;
        loading.loading = true;
        m_overviewPage->setInspection(loading);
        m_browserPage->setInspection(loading);
        m_market->fetchOverview(marketHashName, appid);
    };
    connect(m_overviewPage, &MarketOverviewPage::itemInspectRequested, this, inspectItem);
    connect(m_browserPage, &MarketBrowserPage::itemInspectRequested, this, inspectItem);
    connect(m_market, &MarketService::overviewUpdated, this,
            [this, inspectedName](const QString &marketHashName,
                                  const PriceOverview &overview,
                                  const QString &errorMessage) {
                if (marketHashName != *inspectedName) return;
                MarketInspectionView inspection;
                inspection.stale = overview.stale;
                inspection.errorText = errorMessage;
                inspection.available = errorMessage.isEmpty() && overview.priceLow >= 0.0;
                if (inspection.available) {
                    const QString currency = overview.currency.isEmpty()
                                                 ? m_settings->settings().currency
                                                 : overview.currency;
                    const QString prefix = Currency::displaySymbol(currency);
                    inspection.lowestSellText = prefix + QString::number(overview.priceLow, 'f', 2);
                    inspection.bestSellText = inspection.lowestSellText;
                    if (overview.volume >= 0) {
                        inspection.sellListingsText = QString::number(overview.volume);
                    }
                }
                if (overview.updatedAt.isValid()) {
                    inspection.fetchedAtText = overview.updatedAt.toLocalTime().toString(
                        QStringLiteral("yyyy-MM-dd HH:mm:ss"));
                }
                m_overviewPage->setInspection(inspection);
                m_browserPage->setInspection(inspection);
            });
    auto updatePersonalSummary = [this]() {
        QStringList recent;
        const QVector<WatchlistItem> watched = m_watchlist->items();
        const qsizetype recentCount = qMin<qsizetype>(3, watched.size());
        for (qsizetype i = 0; i < recentCount; ++i) {
            recent.append(watched.at(i).marketHashName);
        }
        m_overviewPage->setPersonalSummary(static_cast<int>(watched.size()),
                                           static_cast<int>(m_alerts->alerts().size()), recent);
    };
    auto followItem = [this, updatePersonalSummary](int appid, const QString &marketHashName) {
        m_watchlist->add(marketHashName, appid, QString());
        updatePersonalSummary();
    };
    connect(m_overviewPage, &MarketOverviewPage::followRequested, this, followItem);
    connect(m_browserPage, &MarketBrowserPage::followRequested, this, followItem);
    auto configureAlert = [this](int appid, const QString &marketHashName) {
        m_mainWindow->showDetail(marketHashName, appid);
    };
    connect(m_overviewPage, &MarketOverviewPage::alertRequested, this, configureAlert);
    connect(m_browserPage, &MarketBrowserPage::alertRequested, this, configureAlert);
    connect(m_watchlist, &WatchlistService::listChanged, this, updatePersonalSummary);
    connect(m_alerts, &AlertService::alertsChanged, this, updatePersonalSummary);
    updatePersonalSummary();
    connect(m_welcomePage, &WelcomePage::guestRequested, this, [this]() {
        m_steamSession->chooseGuest();
        m_mainWindow->setWelcomeVisible(false);
        m_mainWindow->navigateTo(QStringLiteral("overview"));
    });
    connect(m_welcomePage, &WelcomePage::loginRequested, m_steamSession,
            &SteamSessionService::showLogin);
    connect(m_detailPage, &DetailPage::loginRequested, this, [this]() {
        m_historyLoginPending = true;
        m_steamSession->showLogin();
    });
    connect(m_market, &MarketService::historyAuthenticationExpired, this, [this]() {
        if (m_steamSession->isAuthenticated()) m_steamSession->markSessionRejected();
    });
    connect(m_welcomePage, &WelcomePage::rememberGuestChanged, this, [this](bool remember) {
        AppSettings next = m_settings->settings();
        next.startupIdentityMode = remember ? AppSettings::StartupIdentityMode::kGuest
                                            : AppSettings::StartupIdentityMode::kAskEveryTime;
        m_settings->save(next);
    });
    auto *accountCard = m_mainWindow->findChild<AccountCard *>();
    connect(m_steamSession, &SteamSessionService::identityChanged, accountCard,
            &AccountCard::setSnapshot);
    connect(m_steamSession, &SteamSessionService::identityChanged, this,
            [this](const IdentitySnapshot &snapshot) {
                m_welcomePage->setLoginBusy(snapshot.state == IdentityState::Authenticating);
                m_market->setHistoryAuthenticated(snapshot.state == IdentityState::Authenticated);
                if (snapshot.state == IdentityState::Authenticated) {
                    accumulateHistory();  // 登录后立即为自选物品积累历史
                    m_mainWindow->setWelcomeVisible(false);
                    if (m_historyLoginPending) {
                        m_historyLoginPending = false;
                        m_detailPage->refreshHistory();
                    } else {
                        m_mainWindow->navigateTo(QStringLiteral("inventory"));
                    }
                } else if (snapshot.state != IdentityState::Authenticating) {
                    // 注销或会话过期后立即让详情页切换为“缓存 + 登录引导”。
                    m_detailPage->refreshHistory();
                }
            });
    connect(m_steamSession, &SteamSessionService::loginSurfaceClosed, this, [this]() {
        if (!m_steamSession->isAuthenticated()) m_historyLoginPending = false;
    });
    connect(accountCard, &AccountCard::loginRequested, m_steamSession,
            &SteamSessionService::showLogin);
    connect(accountCard, &AccountCard::logoutRequested, this,
            [this]() { m_steamSession->logout(); });
    connect(accountCard, &AccountCard::manageRequested, this, [this]() {
        if (m_steamSession->isAuthenticated()) m_mainWindow->navigateTo(QStringLiteral("inventory"));
        else m_mainWindow->setWelcomeVisible(true);
    });
    accountCard->setSnapshot(m_steamSession->snapshot());
    m_market->setHistoryAuthenticated(m_steamSession->isAuthenticated());
    const bool smoke = QCoreApplication::arguments().contains(QStringLiteral("--smoke-test"));
    const bool startGuest = smoke || m_settings->settings().startupIdentityMode
                                        == AppSettings::StartupIdentityMode::kGuest;
    if (startGuest) {
        m_steamSession->chooseGuest();
        m_mainWindow->setWelcomeVisible(false);
    } else {
        m_mainWindow->setWelcomeVisible(true);
    }
    if (auto *marketPage = m_mainWindow->findChild<MarketPage *>()) {
        marketPage->setGame(m_settings->settings().gameAppid);
    }
    m_notifications->setSink([this](const QString &title, const QString &body) {
        if (m_settings->settings().notificationsEnabled && m_tray) {
            m_tray->showMessage(title, body);
        }
    });
    connect(m_alerts, &AlertService::alertTriggered, m_notifications,
            &NotificationService::notify);
    connect(m_tray, &TrayManager::showRequested, this, [this]() {
        if (m_mainWindow) {
            m_mainWindow->showNormal();
            m_mainWindow->raise();
            m_mainWindow->activateWindow();
        }
    });
    connect(m_tray, &TrayManager::quitRequested, qApp, &QApplication::quit);
    connect(m_settings, &SettingsService::settingsChanged, this, [this](const AppSettings &) {
        applySettings();
        if (auto *marketPage = m_mainWindow->findChild<MarketPage *>()) {
            marketPage->setGame(m_settings->settings().gameAppid);
        }
    });
    connect(m_watchlist, &WatchlistService::refreshFinished, this, [this](const QString &error) {
        m_alerts->checkAll();
        m_portfolio->refreshPrices();
        const QString net = error.isEmpty() ? QStringLiteral("在线") : QStringLiteral("离线/降级");
        const QString sync = QDateTime::currentDateTime().toLocalTime().toString(
            QStringLiteral("HH:mm:ss"));
        if (m_mainWindow) m_mainWindow->setStatus(net, sync);
    });
    // 设置页备份按钮：由 AppController 执行真实备份。
    const auto settingsPage = m_mainWindow->findChild<SettingsPage *>();
    if (settingsPage) {
        connect(settingsPage, &SettingsPage::settingsSaved, this,
                [this](const AppSettings &) { m_dbManager->backup(); });
    }
}

void AppController::startRefreshTimer() {
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, m_watchlist, &WatchlistService::refreshAll);
    m_refreshTimer->setInterval(m_settings->settings().refreshIntervalMinutes * 60000);
    m_refreshTimer->start();
    // 后台历史积累：每 30 分钟为自选物品拉取一次 Steam 官方历史点，
    // K 线、分时与 24h 涨跌幅随运行时间越来越准（仅登录后有效）。
    m_historyTimer = new QTimer(this);
    connect(m_historyTimer, &QTimer::timeout, this, &AppController::accumulateHistory);
    m_historyTimer->setInterval(30 * 60 * 1000);
    m_historyTimer->start();
}

void AppController::accumulateHistory() {
    if (!m_servicesBuilt || !m_market || !m_watchlist) return;
    // Steam 官方 pricehistory 需要已登录会话；未登录时跳过，避免无谓重试。
    if (!m_steamSession->isAuthenticated() || !m_market->isHistoryAuthenticated()) return;
    const QVector<WatchlistItem> list = m_watchlist->items();
    for (const WatchlistItem &it : list) {
        m_market->refreshHistory(it.marketHashName, it.appid);
    }
    if (!list.isEmpty()) {
        qInfo() << "后台历史积累: 已为" << list.size() << "个自选物品排队拉取官方历史";
    }
}

void AppController::applySettings() {
    if (!m_servicesBuilt) return;
    const AppSettings s = m_settings->settings();
    CurrencyProvider::setCode(s.currency);
    ThemeProvider::setScheme(s.colorScheme);
    // 应用级代理：市场接口、库存接口与图标下载等所有网络组件统一生效。
    QNetworkProxy::setApplicationProxy(ProxyConfig::proxyFromSettings(s));
    if (m_nam) {
        m_nam->setProxy(QNetworkProxy::DefaultProxy);
    }
    m_client->setCurrency(s.currency);
    m_client->setRequestIntervalMs(s.requestIntervalMs);
    if (m_overviewPage) m_overviewPage->setCurrency(s.currency);
    if (m_browserPage) m_browserPage->setCurrency(s.currency);
    if (m_refreshTimer) {
        m_refreshTimer->setInterval(s.refreshIntervalMinutes * 60000);
    }
    if (m_mainWindow) {
        m_mainWindow->setCloseToTray(s.trayOnClose);
    }
}

bool AppController::seedDemoData(QSqlDatabase &db) {
    Q_UNUSED(db);
    // 仅冒烟模式使用：写入确定性的演示数据，便于离线验证与截图。
    const QStringList names = {
        QStringLiteral("AK-47 | Redline (Field-Tested)"),
        QStringLiteral("AWP | Asiimov (Battle-Scarred)"),
        QStringLiteral("M4A4 | Howl (Minimal Wear)"),
    };
    for (int i = 0; i < names.size(); ++i) {
        MarketItem item;
        item.marketHashName = names.at(i);
        item.name = names.at(i);
        item.price = 95.0 + i * 3.0;
        item.hasPrice = true;
        item.volume = 300 + i * 100;
        m_items->upsert(item);
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const QString &name : names) {
        for (int d = 5; d >= 0; --d) {
            PriceOverview snap;
            snap.marketHashName = name;
            snap.currency = QStringLiteral("CNY");
            snap.priceLow = 90.0 + d * 1.2 + names.indexOf(name);
            snap.priceHigh = snap.priceLow + 2.0;
            snap.volume = 400 + d * 37;
            snap.updatedAt = now.addDays(-d);
            m_prices->saveSnapshot(snap, 730);
        }
        QVector<PricePoint> hist;
        for (int d = 60; d >= 0; --d) {
            PricePoint p;
            p.recordedAt = now.addDays(-d);
            p.price = 88.0 + qSin(d / 5.0) * 6.0 + names.indexOf(name);
            p.volume = 300 + d * 7;
            hist.append(p);
        }
        m_prices->saveHistory(name, 730, QStringLiteral("CNY"), hist);
    }
    MarketCatalogPage catalog;
    catalog.query.limit = 10;
    catalog.offset = 0;
    catalog.pageSize = static_cast<int>(names.size());
    catalog.totalCount = 527982;
    catalog.fetchedAt = now;
    for (int i = 0; i < names.size(); ++i) {
        MarketCatalogItem item;
        item.marketHashName = names.at(i);
        item.appid = 730;
        item.localizedName = names.at(i);
        item.typeText = QStringLiteral("CS2 smoke fixture");
        item.lowestSellMinor = 9500 + i * 300;
        item.sellListings = 300 + i * 100;
        catalog.items.append(item);
    }
    QString catalogError;
    if (!m_catalogRepo->savePage(catalog, now.addDays(1), &catalogError)) {
        qCritical() << "Smoke catalog seed failed:" << catalogError;
        return false;
    }
    for (const QString &name : names) {
        m_watchlistRepo->add(name, 730, QString());
    }
    PortfolioItem p1;
    p1.marketHashName = names.at(0);
    p1.quantity = 2;
    p1.purchasePrice = 90.0;
    m_portfolioRepo->add(p1);
    PortfolioItem p2;
    p2.marketHashName = names.at(1);
    p2.quantity = 1;
    p2.purchasePrice = 120.0;
    m_portfolioRepo->add(p2);
    Alert alert;
    alert.marketHashName = names.at(0);
    alert.conditionType = Alert::Condition::kBelow;
    alert.thresholdValue = 1000.0;  // 保证命中
    m_alertRepo->add(alert);

    // v2：盘口样例 + 模拟交易
    Orderbook book;
    book.marketHashName = names.at(0);
    for (int i = 0; i < 5; ++i) {
        OrderbookEntry buy;
        buy.price = 91.0 - i * 0.5;
        buy.count = 30 - i * 3;
        book.buyOrders.append(buy);
        OrderbookEntry sell;
        sell.price = 97.0 + i * 0.5;
        sell.count = 20 - i * 2;
        book.sellOrders.append(sell);
    }
    book.highestBuy = book.buyOrders.first().price;
    book.lowestSell = book.sellOrders.first().price;
    book.steamCurrencyId = 23;
    book.fetchedAt = QDateTime::currentDateTimeUtc();
    m_orderbookRepo->save(book);
    m_trades->record(names.at(0), TradeRecord::Side::kBuy, 2, 95.0, QStringLiteral("smoke"));
    m_trades->record(names.at(0), TradeRecord::Side::kSell, 1, 100.0);
    return true;
}

bool AppController::verifyCore(QSqlDatabase &db, int *triggeredCount) {
    Q_UNUSED(db);
    int failures = 0;
    const QVector<WatchlistItem> watch = m_watchlist->items();
    if (watch.size() != 3) {
        qCritical() << "冒烟失败：自选数量应为 3，实际" << watch.size();
        ++failures;
    }
    for (const WatchlistItem &it : watch) {
        if (!it.hasPrice || it.latestPrice <= 0) {
            qCritical() << "冒烟失败：自选项无价格" << it.marketHashName;
            ++failures;
        }
    }
    if (!watch.isEmpty()) {
        const QVector<PricePoint> hist = m_prices->history(watch.first().marketHashName,
                                                           watch.first().appid,
                                                           QStringLiteral("CNY"));
        if (hist.size() < 60) {
            qCritical() << "冒烟失败：历史点数不足" << hist.size();
            ++failures;
        }
    }
    const PortfolioSummary sum = m_portfolio->summary();
    if (sum.itemCount != 2 || sum.missingPriceCount != 0 || sum.totalMarketValue <= 0) {
        qCritical() << "冒烟失败：持仓汇总异常" << sum.itemCount << sum.totalMarketValue;
        ++failures;
    }
    int triggered = 0;
    QMetaObject::Connection conn = connect(
        m_alerts, &AlertService::alertTriggered, this,
        [&triggered](const QString &, const QString &) { ++triggered; });
    m_alerts->checkAll();
    disconnect(conn);
    if (triggered != 1) {
        qCritical() << "冒烟失败：提醒应触发 1 次，实际" << triggered;
        ++failures;
    }
    if (triggeredCount) *triggeredCount = triggered;

    // v2：K 线聚合 / 排行榜 / 费用计算 / 模拟交易
    if (!watch.isEmpty()) {
        const QVector<KlineBar> bars =
            m_kline->dailyBars(watch.first().marketHashName, watch.first().appid,
                               QStringLiteral("CNY"), 90);
        if (bars.size() < 30) {
            qCritical() << "冒烟失败：K线聚合点数不足" << bars.size();
            ++failures;
        }
    }
    if (m_ranking->top(RankingService::By::kVolume, 10).isEmpty()) {
        qCritical() << "冒烟失败：排行榜为空";
        ++failures;
    }
    const FeeEstimate fee = m_rules->estimateFee(QStringLiteral("sell"), 100.0);
    if (qAbs(fee.buyerPays - 115.0) > 0.01 || qAbs(fee.totalFee - 15.0) > 0.01) {
        qCritical() << "冒烟失败：费用计算异常" << fee.buyerPays << fee.totalFee;
        ++failures;
    }
    if (m_trades->records().size() != 2
        || (!watch.isEmpty()
            && m_tradeRepo->holdings(watch.first().marketHashName) != 1)) {
        qCritical() << "冒烟失败：模拟交易记录异常";
        ++failures;
    }
    qInfo() << "冒烟核心检查完成，失败数:" << failures;
    return failures == 0;
}

int AppController::runSmokeTest(const QString &pngPath, const QSize &windowSize,
                                const QString &scene) {
    const QString tmpDir = QDir::tempPath()
                           + QStringLiteral("/smt_smoke_")
                           + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir().mkpath(tmpDir);
    DatabaseManager db(tmpDir + QStringLiteral("/smoke.db"));
    if (!db.open()) {
        qCritical() << "冒烟失败：临时数据库打不开，路径:" << tmpDir
                    << "可用驱动:" << QSqlDatabase::drivers().join(QLatin1Char(','));
        return 1;
    }
    m_db = db.database();
    buildServices(m_db);
    buildUi();
    wireSignals();
    seedDemoData(m_db);
    int triggered = 0;
    const bool coreOk = verifyCore(m_db, &triggered);

    int result = coreOk ? 0 : 1;
    if (!pngPath.isEmpty()) {
        // 展示指定页面并截图，供视觉验收。详情页仅使用临时库中的
        // 确定性数据，同时保留游客态的登录引导。
        m_mainWindow->resize(windowSize);
        const QString normalizedScene = scene.trimmed().toLower();
        const bool hoverDetail = normalizedScene == QStringLiteral("detail-hover");
        if (normalizedScene == QStringLiteral("detail") || hoverDetail) {
            m_mainWindow->showDetail(QStringLiteral("AK-47 | Redline (Field-Tested)"), 730);
        } else if (normalizedScene == QStringLiteral("welcome")) {
            m_mainWindow->setWelcomeVisible(true);
        } else {
            const QString page = normalizedScene.isEmpty()
                                     || normalizedScene == QStringLiteral("trading")
                                 ? QStringLiteral("overview") : normalizedScene;
            m_mainWindow->navigateTo(page);
        }
        m_mainWindow->show();
        if (hoverDetail) {
            QTimer::singleShot(700, [this]() {
                const auto charts = m_detailPage->findChildren<PriceChart *>();
                for (PriceChart *priceChart : charts) {
                    if (!priceChart->isVisible()) continue;
                    QChartView *view = priceChart->findChild<QChartView *>();
                    if (!view || view->chart()->plotArea().isEmpty()) continue;
                    const QPoint position = view->chart()->plotArea().center().toPoint();
                    const QPointF localPosition(position);
                    const QPointF globalPosition(view->viewport()->mapToGlobal(position));
                    QMouseEvent move(QEvent::MouseMove, localPosition, localPosition,
                                     globalPosition, Qt::NoButton, Qt::NoButton,
                                     Qt::NoModifier);
                    QApplication::sendEvent(view->viewport(), &move);
                    break;
                }
            });
        }
        QEventLoop loop;
        QTimer::singleShot(1200, [this, pngPath, normalizedScene, &result, &loop]() {
            if (normalizedScene == QLatin1String("inventory")) {
                QPushButton *batchAction = m_mainWindow->findChild<QPushButton *>(
                    QStringLiteral("batchListingVisibleAction"));
                const bool centerOnScreen = batchAction
                    && m_mainWindow->rect().contains(
                        batchAction->mapTo(m_mainWindow, batchAction->rect().center()));
                if (!batchAction || !batchAction->isVisibleTo(m_mainWindow)
                    || !centerOnScreen) {
                    qCritical() << "冒烟失败：库存首屏缺少可见的批量上架入口";
                    result = 1;
                }
            }
            const QPixmap shot = m_mainWindow->grab();
            if (!shot.save(pngPath)) {
                qCritical() << "冒烟失败：截图保存失败" << pngPath;
                result = 1;
            } else {
                qInfo() << "冒烟截图已保存:" << pngPath;
            }
            loop.quit();
        });
        loop.exec();
    }
    releaseRepositories();
    db.close();
    QDir(tmpDir).removeRecursively();
    return result;
}
