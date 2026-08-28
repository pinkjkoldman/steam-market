TEMPLATE = app
TARGET = tests
QT += core gui sql network testlib
CONFIG += c++17 console
CONFIG -= app_bundle

# 与主工程一致：严格警告作为发布质量门槛
QMAKE_CXXFLAGS_WARN_ON += -Wextra

DESTDIR = $$PWD/../bin
OBJECTS_DIR = $$PWD/../build/test_obj
MOC_DIR = $$PWD/../build/test_moc
RCC_DIR = $$PWD/../build/test_rcc

INCLUDEPATH += $$PWD/../src $$PWD

SOURCES += \
    main.cpp \
    test_database.cpp \
    test_alerts.cpp \
    test_portfolio.cpp \
    test_csv.cpp \
    test_kline.cpp \
    test_fee.cpp \
    test_trades.cpp \
    test_orderbook.cpp \
    test_inventory.cpp \
    ../src/data/DatabaseManager.cpp \
    ../src/data/repositories/ItemRepository.cpp \
    ../src/data/repositories/InventoryRepository.cpp \
    ../src/data/repositories/PriceRepository.cpp \
    ../src/data/repositories/WatchlistRepository.cpp \
    ../src/data/repositories/AlertRepository.cpp \
    ../src/data/repositories/PortfolioRepository.cpp \
    ../src/data/repositories/SettingsRepository.cpp \
    ../src/data/repositories/OrderbookRepository.cpp \
    ../src/data/repositories/TradeRepository.cpp \
    ../src/core/services/AlertService.cpp \
    ../src/core/services/PortfolioService.cpp \
    ../src/core/services/SettingsService.cpp \
    ../src/core/services/PriceSourceService.cpp \
    ../src/core/services/KlineService.cpp \
    ../src/core/services/TradingRulesService.cpp \
    ../src/core/services/TradeSimulationService.cpp \
    ../src/core/services/PricingDraftService.cpp \
    ../src/core/services/MultiSellHandoffService.cpp \
    ../src/core/services/TradeHandoffService.cpp \
    ../src/core/services/SteamSessionService.cpp \
    ../src/network/InventoryParser.cpp \
    ../src/network/SteamInventoryClient.cpp \
    ../src/network/SteamMarketClient.cpp \
    ../src/network/SteamOrderbookParser.cpp \
    ../src/network/SteamHistoryParser.cpp \
    ../src/price_sources/CsvPriceSource.cpp \
    ../src/network/SteamMarketCatalogClient.cpp \
    ../src/data/repositories/MarketCatalogRepository.cpp \
    ../src/core/services/FullMarketService.cpp \
    ../src/core/services/AccessGate.cpp \
    ../src/ui/widgets/MarketPageFilterEngine.cpp \
    test_full_market.cpp \
    test_account_entry.cpp \
    test_steam_history_parser.cpp \
    test_steam_orderbook_parser.cpp

HEADERS += \
    test_database.h \
    test_alerts.h \
    test_portfolio.h \
    test_csv.h \
    test_kline.h \
    test_fee.h \
    test_trades.h \
    test_orderbook.h \
    test_inventory.h \
    test_helpers.h \
    ../src/data/DatabaseManager.h \
    ../src/data/repositories/ItemRepository.h \
    ../src/data/repositories/InventoryRepository.h \
    ../src/data/repositories/PriceRepository.h \
    ../src/data/repositories/WatchlistRepository.h \
    ../src/data/repositories/AlertRepository.h \
    ../src/data/repositories/PortfolioRepository.h \
    ../src/data/repositories/SettingsRepository.h \
    ../src/data/repositories/OrderbookRepository.h \
    ../src/data/repositories/TradeRepository.h \
    ../src/core/services/AlertService.h \
    ../src/core/services/PortfolioService.h \
    ../src/core/services/SettingsService.h \
    ../src/core/services/PriceSourceService.h \
    ../src/core/services/KlineService.h \
    ../src/core/services/TradingRulesService.h \
    ../src/core/services/TradeSimulationService.h \
    ../src/core/services/PricingDraftService.h \
    ../src/core/services/MultiSellHandoffService.h \
    ../src/core/services/TradeHandoffService.h \
    ../src/core/services/SteamSessionService.h \
    ../src/network/IWebSessionHost.h \
    ../src/network/InventoryParser.h \
    ../src/network/SteamInventoryClient.h \
    ../src/network/SteamMarketClient.h \
    ../src/network/SteamOrderbookParser.h \
    ../src/network/SteamHistoryParser.h \
    ../src/network/RateLimiter.h \
    ../src/price_sources/CsvPriceSource.h \
    ../src/core/errors.h \
    ../src/core/models/MarketItem.h \
    ../src/core/models/PriceOverview.h \
    ../src/core/models/PricePoint.h \
    ../src/core/models/WatchlistItem.h \
    ../src/core/models/Alert.h \
    ../src/core/models/PortfolioItem.h \
    ../src/core/models/PlatformPrice.h \
    ../src/core/models/AppSettings.h \
    ../src/core/models/KlineBar.h \
    ../src/core/models/Orderbook.h \
    ../src/core/models/FeeEstimate.h \
    ../src/core/models/TradeRecord.h \
    ../src/core/models/InventoryModels.h \
    ../src/network/SteamMarketCatalogClient.h \
    ../src/data/repositories/MarketCatalogRepository.h \
    ../src/core/models/MarketCatalog.h \
    ../src/core/models/IdentityAccount.h \
    ../src/core/models/IdentitySnapshot.h \
    ../src/core/services/FullMarketService.h \
    ../src/core/services/AccessGate.h \
    ../src/ui/widgets/MarketPageFilterEngine.h \
    ../src/ui/widgets/WorkbenchModels.h \
    test_full_market.h \
    test_account_entry.h \
    test_steam_history_parser.h \
    test_steam_orderbook_parser.h


RESOURCES += ../src/data/migrations.qrc
