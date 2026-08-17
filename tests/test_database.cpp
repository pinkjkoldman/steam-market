#include "test_database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QtTest>

#include "data/repositories/AlertRepository.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/PortfolioRepository.h"
#include "data/repositories/PriceRepository.h"
#include "data/repositories/SettingsRepository.h"
#include "data/repositories/WatchlistRepository.h"
#include "test_helpers.h"

void TestDatabase::migrationApplied() {
    TestDb f;
    QVERIFY(f.opened);
    QSqlQuery q(f.handle);
    QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM schema_migrations")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 6);  // v1~v6 迁移
    QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE type='table' "
                                  "AND name IN ('items','price_snapshots','price_history',"
                                  "'watchlist','alerts','portfolio_items','platform_prices','settings')")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 8);
}

void TestDatabase::itemUpsert() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository repo(f.handle);
    MarketItem item;
    item.marketHashName = QStringLiteral("AK-47 | Redline");
    item.name = item.marketHashName;
    item.iconUrl = QStringLiteral("http://example/icon.png");
    QVERIFY(repo.upsert(item));
    QCOMPARE(repo.iconUrlOf(item.marketHashName), item.iconUrl);
    QVERIFY(repo.upsert(item));  // 幂等
}

void TestDatabase::priceSnapshotAndHistory() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    PriceRepository repo(f.handle);
    MarketItem item;
    item.marketHashName = QStringLiteral("AWP | Asiimov");
    item.name = item.marketHashName;
    items.upsert(item);

    PriceOverview snap;
    snap.marketHashName = item.marketHashName;
    snap.currency = QStringLiteral("CNY");
    snap.priceLow = 99.5;
    snap.priceHigh = 101.25;
    snap.volume = 1234;
    snap.updatedAt = QDateTime::currentDateTimeUtc();
    QVERIFY(repo.saveSnapshot(snap, 730));

    const PriceOverview read = repo.latestSnapshot(item.marketHashName, 730, QStringLiteral("CNY"));
    QCOMPARE(read.priceHigh, 101.25);
    QCOMPARE(read.volume, 1234);
    QVERIFY(read.stale);

    QVector<PricePoint> hist;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (int i = 0; i < 5; ++i) {
        PricePoint p;
        p.recordedAt = now.addDays(-i);
        p.price = 90.0 + i;
        p.volume = 100 + i;
        hist.append(p);
    }
    QVERIFY(repo.saveHistory(item.marketHashName, 730, QStringLiteral("CNY"), hist));
    QCOMPARE(repo.history(item.marketHashName, 730, QStringLiteral("CNY")).size(), 5);
}

void TestDatabase::watchlistCrud() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    WatchlistRepository repo(f.handle);
    MarketItem item;
    item.marketHashName = QStringLiteral("M4A4 | Howl");
    item.name = item.marketHashName;
    items.upsert(item);
    QVERIFY(repo.add(item.marketHashName, 730, QStringLiteral("关注")));
    QVERIFY(repo.contains(item.marketHashName));
    QCOMPARE(repo.all().size(), 1);
    QVERIFY(!repo.add(item.marketHashName, 730, QString()));  // 唯一约束去重
    QVERIFY(repo.remove(item.marketHashName));
    QVERIFY(!repo.contains(item.marketHashName));
}

void TestDatabase::alertCrud() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    AlertRepository repo(f.handle);
    MarketItem item;
    item.marketHashName = QStringLiteral("Glock | Fade");
    item.name = item.marketHashName;
    items.upsert(item);
    Alert a;
    a.marketHashName = item.marketHashName;
    a.conditionType = Alert::Condition::kBelow;
    a.thresholdValue = 50.0;
    QVERIFY(repo.add(a));
    QCOMPARE(repo.all().size(), 1);
    QCOMPARE(repo.enabled().size(), 1);
    Alert read = repo.all().first();
    read.enabled = false;
    QVERIFY(repo.update(read));
    QCOMPARE(repo.enabled().size(), 0);
    QVERIFY(repo.markTriggered(read.id, QDateTime::currentDateTimeUtc()));
    QVERIFY(!repo.all().first().lastTriggeredAt.isNull());
    QVERIFY(repo.remove(read.id));
    QCOMPARE(repo.all().size(), 0);
}

void TestDatabase::portfolioCrud() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    PortfolioRepository repo(f.handle);
    MarketItem item;
    item.marketHashName = QStringLiteral("Karambit | Doppler");
    item.name = item.marketHashName;
    items.upsert(item);
    PortfolioItem p;
    p.marketHashName = item.marketHashName;
    p.quantity = 3;
    p.purchasePrice = 100.0;
    QVERIFY(repo.add(p));
    QCOMPARE(repo.all().size(), 1);
    QCOMPARE(repo.marketHashNames(), QStringList{item.marketHashName});
    PortfolioItem read = repo.all().first();
    read.quantity = 5;
    QVERIFY(repo.update(read));
    QCOMPARE(repo.all().first().quantity, 5);
    QVERIFY(repo.remove(read.id));
    QVERIFY(repo.all().isEmpty());
}

void TestDatabase::settingsRoundtrip() {
    TestDb f;
    QVERIFY(f.opened);
    SettingsRepository repo(f.handle);
    QVERIFY(repo.setValue(QStringLiteral("currency"), QStringLiteral("USD")));
    QCOMPARE(repo.value(QStringLiteral("currency")), QStringLiteral("USD"));
    QCOMPARE(repo.value(QStringLiteral("missing"), QStringLiteral("default")),
             QStringLiteral("default"));
}
