#include "test_trades.h"

#include <QtTest>

#include "core/services/PortfolioService.h"
#include "core/services/SettingsService.h"
#include "core/services/TradeSimulationService.h"
#include "core/services/TradingRulesService.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/PortfolioRepository.h"
#include "data/repositories/PriceRepository.h"
#include "data/repositories/SettingsRepository.h"
#include "data/repositories/TradeRepository.h"
#include "test_helpers.h"

void TestTrades::buySellLifecycle() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    PriceRepository prices(f.handle);
    PortfolioRepository portfolioRepo(f.handle);
    TradeRepository tradeRepo(f.handle);
    SettingsRepository settingsRepo(f.handle);
    SettingsService settings(&settingsRepo);
    TradingRulesService rules(&settings);
    PortfolioService portfolio(&portfolioRepo, &prices, &items);
    TradeSimulationService trades(&tradeRepo, &items, &rules, &portfolio);

    const QString name = QStringLiteral("TRADE1");
    QVERIFY(trades.record(name, TradeRecord::Side::kBuy, 2, 100.0).isEmpty());
    QVERIFY(trades.record(name, TradeRecord::Side::kSell, 1, 100.0).isEmpty());

    const QVector<TradeRecord> records = trades.records();
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.at(0).side, TradeRecord::Side::kSell);  // 按时间倒序
    QCOMPARE(records.at(0).fee, 15.0);                        // 卖出 100 手续费 15
    QCOMPARE(records.at(0).total, 85.0);                      // 净得
    QCOMPARE(records.at(1).side, TradeRecord::Side::kBuy);
    // 买入 200 按"买家支付→卖家到手"折算手续费 = 200 - 200/1.15 ≈ 26.09
    QVERIFY(qAbs(records.at(1).fee - 26.09) < 0.01);
    QVERIFY(qAbs(records.at(1).total - 226.09) < 0.01);
    QCOMPARE(tradeRepo.holdings(name), 1);
}

void TestTrades::oversellRejected() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    PriceRepository prices(f.handle);
    PortfolioRepository portfolioRepo(f.handle);
    TradeRepository tradeRepo(f.handle);
    SettingsRepository settingsRepo(f.handle);
    SettingsService settings(&settingsRepo);
    TradingRulesService rules(&settings);
    PortfolioService portfolio(&portfolioRepo, &prices, &items);
    TradeSimulationService trades(&tradeRepo, &items, &rules, &portfolio);

    const QString name = QStringLiteral("TRADE2");
    QVERIFY(trades.record(name, TradeRecord::Side::kBuy, 1, 100.0).isEmpty());
    QVERIFY(!trades.record(name, TradeRecord::Side::kSell, 2, 100.0).isEmpty());
    QCOMPARE(trades.records().size(), 1);
}
