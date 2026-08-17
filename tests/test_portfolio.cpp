#include "test_portfolio.h"

#include <QtTest>

#include "core/services/PortfolioService.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/PortfolioRepository.h"
#include "data/repositories/PriceRepository.h"
#include "test_helpers.h"

void TestPortfolio::valuationMath() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    PriceRepository prices(f.handle);
    PortfolioRepository portfolio(f.handle);
    PortfolioService service(&portfolio, &prices, &items);

    const QString name = QStringLiteral("PORT1");
    MarketItem item;
    item.marketHashName = name;
    item.name = name;
    items.upsert(item);
    PriceOverview snap;
    snap.marketHashName = name;
    snap.currency = QStringLiteral("CNY");
    snap.priceHigh = 50.0;
    snap.updatedAt = QDateTime::currentDateTimeUtc();
    prices.saveSnapshot(snap, 730);

    PortfolioItem p;
    p.marketHashName = name;
    p.quantity = 4;
    p.purchasePrice = 40.0;
    portfolio.add(p);

    const PortfolioItem read = service.items().first();
    QVERIFY(read.hasPrice);
    QCOMPARE(read.latestPrice, 50.0);
    QCOMPARE(read.marketValue, 200.0);
    QCOMPARE(read.profitLoss, 40.0);

    const PortfolioSummary s = service.summary();
    QCOMPARE(s.itemCount, 1);
    QCOMPARE(s.missingPriceCount, 0);
    QCOMPARE(s.totalMarketValue, 200.0);
    QCOMPARE(s.totalCost, 160.0);
    QCOMPARE(s.totalProfitLoss, 40.0);
    QVERIFY(qAbs(s.totalProfitLossPercent - 25.0) < 0.001);
}

void TestPortfolio::missingPriceExcluded() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    PriceRepository prices(f.handle);
    PortfolioRepository portfolio(f.handle);
    PortfolioService service(&portfolio, &prices, &items);

    MarketItem item;
    item.marketHashName = QStringLiteral("NO_PRICE");
    item.name = item.marketHashName;
    QVERIFY(items.upsert(item));

    PortfolioItem p;
    p.marketHashName = QStringLiteral("NO_PRICE");
    p.quantity = 2;
    QVERIFY(portfolio.add(p));

    const PortfolioItem read = service.items().first();
    QVERIFY(!read.hasPrice);
    const PortfolioSummary s = service.summary();
    QCOMPARE(s.missingPriceCount, 1);
    QCOMPARE(s.totalMarketValue, 0.0);
}
