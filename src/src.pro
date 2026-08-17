TEMPLATE = app
TARGET = SteamMarketTerminal
QT += core gui widgets network sql charts
CONFIG += c++17
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = $$PWD/../bin
OBJECTS_DIR = $$PWD/../build/obj
MOC_DIR = $$PWD/../build/moc
RCC_DIR = $$PWD/../build/rcc

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../third_party/webview2

SOURCES += \
    main.cpp \
    app/AppController.cpp \
    app/MainWindow.cpp \
    core/services/MarketService.cpp \
    core/services/WatchlistService.cpp \
    core/services/AlertService.cpp \
    core/services/PortfolioService.cpp \
    core/services/PriceSourceService.cpp \
    core/services/SettingsService.cpp \
    core/services/NotificationService.cpp \
    core/services/KlineService.cpp \
    core/services/OrderbookService.cpp \
    core/services/RankingService.cpp \
    core/services/TradingRulesService.cpp \
    core/services/TradeSimulationService.cpp \
    core/services/SteamSessionService.cpp \
    core/services/InventoryService.cpp \
    core/services/PricingDraftService.cpp \
    core/services/MultiSellHandoffService.cpp \
    data/DatabaseManager.cpp \
    data/repositories/ItemRepository.cpp \
    data/repositories/InventoryRepository.cpp \
    data/repositories/PriceRepository.cpp \
    data/repositories/WatchlistRepository.cpp \
    data/repositories/AlertRepository.cpp \
    data/repositories/PortfolioRepository.cpp \
    data/repositories/OrderbookRepository.cpp \
    data/repositories/SettingsRepository.cpp \
    data/repositories/TradeRepository.cpp \
    network/SteamMarketClient.cpp \
    network/SteamInventoryClient.cpp \
    network/InventoryParser.cpp \
    network/WebView2SessionHost.cpp \
    price_sources/SteamPriceSource.cpp \
    price_sources/CsvPriceSource.cpp \
    ui/pages/MarketPage.cpp \
    ui/pages/DetailPage.cpp \
    ui/pages/WatchlistPage.cpp \
    ui/pages/PortfolioPage.cpp \
    ui/pages/RankingPage.cpp \
    ui/pages/RulesPage.cpp \
    ui/pages/InventoryAssistantPage.cpp \
    ui/pages/InventoryAssistantPageUi.cpp \
    ui/pages/SettingsPage.cpp \
    ui/widgets/PriceChart.cpp \
    ui/widgets/KlineChart.cpp \
    ui/widgets/OrderbookPanel.cpp \
    ui/TrayManager.cpp \
    utils/Logger.cpp \
    ui/widgets/QuickInspector.cpp \
    ui/widgets/ScopeNotice.cpp \
    ui/widgets/AccountCard.cpp \
    ui/widgets/ModeCard.cpp \
    ui/pages/MarketBrowserPage.cpp \
    ui/pages/MarketOverviewPage.cpp \
    ui/pages/WelcomePage.cpp \
    network/SteamMarketCatalogClient.cpp \
    data/repositories/MarketCatalogRepository.cpp \
    core/services/AccessGate.cpp \
    core/services/FullMarketService.cpp

HEADERS += \
    app/AppController.h \
    app/MainWindow.h \
    core/errors.h \
    core/models/MarketItem.h \
    core/models/PriceOverview.h \
    core/models/PricePoint.h \
    core/models/WatchlistItem.h \
    core/models/Alert.h \
    core/models/PortfolioItem.h \
    core/models/PlatformPrice.h \
    core/models/AppSettings.h \
    core/models/KlineBar.h \
    core/models/Orderbook.h \
    core/models/TopItem.h \
    core/models/TradingRule.h \
    core/models/FeeEstimate.h \
    core/models/TradeRecord.h \
    core/models/InventoryModels.h \
    core/services/MarketService.h \
    core/services/WatchlistService.h \
    core/services/AlertService.h \
    core/services/PortfolioService.h \
    core/services/PriceSourceService.h \
    core/services/SettingsService.h \
    core/services/NotificationService.h \
    core/services/KlineService.h \
    core/services/OrderbookService.h \
    core/services/RankingService.h \
    core/services/TradingRulesService.h \
    core/services/TradeSimulationService.h \
    core/services/SteamSessionService.h \
    core/services/InventoryService.h \
    core/services/PricingDraftService.h \
    core/services/MultiSellHandoffService.h \
    data/DatabaseManager.h \
    data/repositories/ItemRepository.h \
    data/repositories/InventoryRepository.h \
    data/repositories/PriceRepository.h \
    data/repositories/WatchlistRepository.h \
    data/repositories/AlertRepository.h \
    data/repositories/PortfolioRepository.h \
    data/repositories/OrderbookRepository.h \
    data/repositories/SettingsRepository.h \
    data/repositories/TradeRepository.h \
    network/SteamMarketClient.h \
    network/SteamInventoryClient.h \
    network/InventoryParser.h \
    network/IWebSessionHost.h \
    network/WebView2SessionHost.h \
    network/RateLimiter.h \
    price_sources/IPriceSource.h \
    price_sources/SteamPriceSource.h \
    price_sources/CsvPriceSource.h \
    ui/pages/MarketPage.h \
    ui/pages/DetailPage.h \
    ui/pages/WatchlistPage.h \
    ui/pages/PortfolioPage.h \
    ui/pages/RankingPage.h \
    ui/pages/RulesPage.h \
    ui/pages/InventoryAssistantPage.h \
    ui/pages/SettingsPage.h \
    ui/widgets/PriceChart.h \
    ui/widgets/KlineChart.h \
    ui/widgets/OrderbookPanel.h \
    ui/TrayManager.h \
    utils/Logger.h \
    utils/Currency.h \
    utils/CurrencyProvider.h \
    utils/ThemeProvider.h \
    ui/widgets/QuickInspector.h \
    ui/widgets/ScopeNotice.h \
    ui/widgets/AccountCard.h \
    ui/widgets/ModeCard.h \
    ui/widgets/WorkbenchTheme.h \
    ui/widgets/WorkbenchModels.h \
    ui/pages/MarketBrowserPage.h \
    ui/pages/MarketOverviewPage.h \
    ui/pages/WelcomePage.h \
    network/SteamMarketCatalogClient.h \
    data/repositories/MarketCatalogRepository.h \
    core/services/AccessGate.h \
    core/services/FullMarketService.h \
    core/models/IdentityAccount.h \
    core/models/IdentitySnapshot.h \
    core/models/MarketCatalog.h

RESOURCES += data/migrations.qrc
RESOURCES += data/rules.qrc
LIBS += -lole32 -luuid

WEBVIEW2_LOADER = $$system_path($$PWD/../third_party/webview2/WebView2Loader.dll)
WEBVIEW2_DEST = $$system_path($$DESTDIR/WebView2Loader.dll)
QMAKE_POST_LINK += $$quote(cmd /c copy /Y "$$WEBVIEW2_LOADER" "$$WEBVIEW2_DEST")
