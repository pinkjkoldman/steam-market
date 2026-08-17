#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSize>
#include <QString>

class DatabaseManager;
class ItemRepository;
class PriceRepository;
class WatchlistRepository;
class AlertRepository;
class PortfolioRepository;
class SettingsRepository;
class OrderbookRepository;
class TradeRepository;
class SteamMarketClient;
class SteamMarketCatalogClient;
class MarketCatalogRepository;
class FullMarketService;
class SteamInventoryClient;
class IWebSessionHost;
class QNetworkAccessManager;
class QNetworkCookieJar;
class MarketService;
class WatchlistService;
class AlertService;
class PortfolioService;
class PriceSourceService;
class SettingsService;
class TradingRulesService;
class KlineService;
class OrderbookService;
class RankingService;
class TradeSimulationService;
class NotificationService;
class MainWindow;
class RankingPage;
class RulesPage;
class InventoryAssistantPage;
class InventoryRepository;
class InventoryService;
class SteamSessionService;
class PricingDraftService;
class MultiSellHandoffService;
class QTimer;
class QSystemTrayIcon;
class TrayManager;
class WelcomePage;
class MarketOverviewPage;
class MarketBrowserPage;

// 应用装配器：初始化数据库/服务/UI，驱动定时刷新与通知；含冒烟自测。
class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    bool initialize();
    void show();
    int runSmokeTest(const QString &pngPath, const QSize &windowSize = QSize(1280, 800));

private:
    void buildServices(QSqlDatabase &db);
    void buildUi();
    void wireSignals();
    void startRefreshTimer();
    void applySettings();
    void releaseRepositories();
    bool seedDemoData(QSqlDatabase &db);
    bool verifyCore(QSqlDatabase &db, int *triggeredCount);

    DatabaseManager *m_dbManager = nullptr;
    ItemRepository *m_items = nullptr;
    InventoryRepository *m_inventoryRepo = nullptr;
    PriceRepository *m_prices = nullptr;
    WatchlistRepository *m_watchlistRepo = nullptr;
    AlertRepository *m_alertRepo = nullptr;
    PortfolioRepository *m_portfolioRepo = nullptr;
    SettingsRepository *m_settingsRepo = nullptr;
    OrderbookRepository *m_orderbookRepo = nullptr;
    TradeRepository *m_tradeRepo = nullptr;
    MarketCatalogRepository *m_catalogRepo = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkCookieJar *m_jar = nullptr;
    SteamMarketClient *m_client = nullptr;
    SteamMarketCatalogClient *m_catalogClient = nullptr;
    FullMarketService *m_fullMarket = nullptr;
    SteamInventoryClient *m_inventoryClient = nullptr;
    IWebSessionHost *m_webSessionHost = nullptr;
    SteamSessionService *m_steamSession = nullptr;
    InventoryService *m_inventoryService = nullptr;
    PricingDraftService *m_pricingDraft = nullptr;
    MultiSellHandoffService *m_handoff = nullptr;
    MarketService *m_market = nullptr;
    WatchlistService *m_watchlist = nullptr;
    AlertService *m_alerts = nullptr;
    PortfolioService *m_portfolio = nullptr;
    PriceSourceService *m_sources = nullptr;
    SettingsService *m_settings = nullptr;
    TradingRulesService *m_rules = nullptr;
    KlineService *m_kline = nullptr;
    OrderbookService *m_orderbook = nullptr;
    RankingService *m_ranking = nullptr;
    TradeSimulationService *m_trades = nullptr;
    NotificationService *m_notifications = nullptr;
    MainWindow *m_mainWindow = nullptr;
    WelcomePage *m_welcomePage = nullptr;
    MarketOverviewPage *m_overviewPage = nullptr;
    MarketBrowserPage *m_browserPage = nullptr;
    TrayManager *m_tray = nullptr;
    QTimer *m_refreshTimer = nullptr;
    bool m_servicesBuilt = false;
    QSqlDatabase m_db;
};
