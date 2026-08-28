#include "test_full_market.h"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "data/repositories/MarketCatalogRepository.h"
#include "network/SteamMarketCatalogClient.h"
#include "test_helpers.h"
#include "ui/widgets/MarketPageFilterEngine.h"

namespace {
QByteArray validCatalogFixture() {
    return R"json({
      "success": true,
      "start": 0,
      "pagesize": 10,
      "total_count": 527988,
      "results": [{
        "name": "反恐精英武器箱",
        "hash_name": "CS:GO Weapon Case",
        "sell_listings": 3210,
        "sell_price": 88123,
        "sale_price_text": "¥ 881.23",
        "asset_description": {
          "appid": 730,
          "type": "普通级武器箱",
          "icon_url": "fixtureIconHash"
        }
      }]
    })json";
}

QDateTime fixtureTime() {
    return QDateTime::fromString(QStringLiteral("2026-08-13T04:00:00Z"), Qt::ISODate);
}

MarketCatalogPage parsedFixture() {
    MarketCatalogQuery query;
    query.appid = 730;
    MarketCatalogPage page;
    MarketCatalogError error;
    const bool ok = SteamMarketCatalogClient::parseResponse(
        validCatalogFixture(), query, fixtureTime(), &page, &error);
    Q_ASSERT(ok && error.isOk());
    return page;
}
}  // namespace

void TestFullMarket::parsesFixedSteamFixture() {
    MarketCatalogQuery query;
    query.appid = 730;
    MarketCatalogPage page;
    MarketCatalogError error;
    QVERIFY(SteamMarketCatalogClient::parseResponse(validCatalogFixture(), query,
                                                     fixtureTime(), &page, &error));
    QVERIFY(error.isOk());
    QCOMPARE(page.totalCount, qint64(527988));
    QCOMPARE(page.pageSize, 10);
    QCOMPARE(page.items.size(), 1);
    QCOMPARE(page.items.first().appid, 730);
    QCOMPARE(page.items.first().marketHashName, QStringLiteral("CS:GO Weapon Case"));
    QCOMPARE(page.items.first().localizedName, QStringLiteral("反恐精英武器箱"));
    QCOMPARE(page.items.first().lowestSellMinor, qint64(88123));
    QCOMPARE(page.items.first().sellListings, qint64(3210));
    QCOMPARE(page.origin, DataOrigin::kSteamLive);
    QVERIFY(!page.stale);
}

void TestFullMarket::rejectsChangedSchemaAtomically() {
    QByteArray fixture = validCatalogFixture();
    fixture.replace("\"sell_price\": 88123", "\"sell_price\": -1");
    MarketCatalogPage page;
    MarketCatalogError error;
    QVERIFY(!SteamMarketCatalogClient::parseResponse(
        fixture, MarketCatalogQuery(), fixtureTime(), &page, &error));
    QCOMPARE(error.stableCode(), QStringLiteral("MARKET_SCHEMA_CHANGED"));
    QVERIFY(page.items.isEmpty());

    fixture = validCatalogFixture();
    fixture.replace("\"appid\": 730", "\"missing_appid\": 730");
    QVERIFY(!SteamMarketCatalogClient::parseResponse(
        fixture, MarketCatalogQuery(), fixtureTime(), &page, &error));
    QCOMPARE(error.stableCode(), QStringLiteral("MARKET_SCHEMA_CHANGED"));
}

void TestFullMarket::validatesQueryAndUrl() {
    MarketCatalogQuery query;
    query.query = QStringLiteral("knife");
    query.appid = 730;
    query.offset = 20;
    query.currency = QStringLiteral("USD");
    query.language = QStringLiteral("english");
    query.sort = CatalogSort::kPriceAsc;
    QVERIFY(SteamMarketCatalogClient::validateQuery(query).isOk());
    const QUrl url = SteamMarketCatalogClient::requestUrl(query);
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("steamcommunity.com"));
    QCOMPARE(url.path(), QStringLiteral("/market/search/render/"));
    const QString decoded = url.query(QUrl::FullyDecoded);
    QVERIFY(decoded.contains(QStringLiteral("start=20")));
    QVERIFY(decoded.contains(QStringLiteral("count=10")));
    QVERIFY(decoded.contains(QStringLiteral("currency=1")));

    query.currency = QStringLiteral("EUR");
    QCOMPARE(SteamMarketCatalogClient::validateQuery(query).stableCode(),
             QStringLiteral("MARKET_CURRENCY_UNSUPPORTED"));
}

void TestFullMarket::boundsRetryAfterDelay() {
    QCOMPARE(SteamMarketCatalogClient::retryAfterDelayMs("120"), qint64(120000));
    QCOMPARE(SteamMarketCatalogClient::retryAfterDelayMs(""), qint64(30000));
    QCOMPARE(SteamMarketCatalogClient::retryAfterDelayMs("-1"), qint64(30000));
    QCOMPARE(SteamMarketCatalogClient::retryAfterDelayMs("9223372036854775807"),
             qint64(24 * 60 * 60 * 1000));
}

void TestFullMarket::filtersCurrentPageAndBuildsVisualSummary() {
    QVector<MarketCatalogItemView> items;
    items.append({730, QStringLiteral("Case A"), QStringLiteral("武器箱 A"),
                  QStringLiteral("普通级武器箱"), QString(), 100, 50});
    items.append({730, QStringLiteral("Case B"), QStringLiteral("武器箱 B"),
                  QStringLiteral("稀有武器箱"), QString(), 300, 500});
    items.append({730, QStringLiteral("Sticker A"), QStringLiteral("印花 A"),
                  QStringLiteral("高级印花"), QString(), 200, 1000});
    items.append({730, QStringLiteral("Unpriced"), QStringLiteral("暂无报价"),
                  QStringLiteral("稀有武器箱"), QString(), 0, 0});

    MarketPageFilter filter;
    filter.typeContains = QStringLiteral("武器箱");
    filter.minimumPriceMinor = 100;
    filter.maximumPriceMinor = 350;
    filter.minimumListings = 40;
    filter.pricedOnly = true;
    const MarketPageFilterResult result = MarketPageFilterEngine::apply(items, filter);

    QCOMPARE(result.items.size(), 2);
    QCOMPARE(result.items.first().marketHashName, QStringLiteral("Case A"));
    QCOMPARE(result.minimumPriceMinor, qint64(100));
    QCOMPARE(result.maximumPriceMinor, qint64(300));
    QCOMPARE(result.medianPriceMinor, qint64(200));
    QCOMPARE(result.totalListings, qint64(550));
}

void TestFullMarket::cacheKeySeparatesDimensions() {
    MarketCatalogQuery base;
    const QString baseKey = MarketCatalogRepository::cacheKey(base);
    MarketCatalogQuery changed = base;
    changed.offset = 10;
    QVERIFY(MarketCatalogRepository::cacheKey(changed) != baseKey);
    changed = base;
    changed.currency = QStringLiteral("USD");
    QVERIFY(MarketCatalogRepository::cacheKey(changed) != baseKey);
    changed = base;
    changed.language = QStringLiteral("english");
    QVERIFY(MarketCatalogRepository::cacheKey(changed) != baseKey);
    changed = base;
    changed.appid = 730;
    QVERIFY(MarketCatalogRepository::cacheKey(changed) != baseKey);
}

void TestFullMarket::repositoryRoundtripAndSnapshotDeduplication() {
    TestDb fixture;
    QVERIFY(fixture.opened);
    QSqlQuery schema(fixture.handle);
    QVERIFY2(schema.exec(QStringLiteral("SELECT cache_key FROM market_catalog_pages LIMIT 1")),
             qPrintable(schema.lastError().text()));
    MarketCatalogRepository repository(fixture.handle);
    MarketCatalogPage live = parsedFixture();
    QString error;
    QVERIFY2(repository.savePage(live, live.fetchedAt.addSecs(600), &error),
             qPrintable(error));
    live.totalCount = 527989;
    QVERIFY2(repository.savePage(live, live.fetchedAt.addSecs(600), &error),
             qPrintable(error));
    const std::optional<MarketCatalogPage> cached = repository.page(live.query, true, &error);
    QVERIFY2(cached.has_value(), qPrintable(error));
    QCOMPARE(cached->origin, DataOrigin::kSteamCached);
    QCOMPARE(cached->totalCount, live.totalCount);
    QCOMPARE(cached->items.first().lowestSellMinor, live.items.first().lowestSellMinor);
    const QVector<MarketScopeSnapshot> scopes =
        repository.latestScopeSnapshots({QStringLiteral("appid:730")});
    QCOMPARE(scopes.size(), 1);
    QCOMPARE(scopes.first().totalCount, live.totalCount);
    QCOMPARE(scopes.first().origin, DataOrigin::kSteamCached);

    MarketCatalogPage searchPage = live;
    searchPage.query.query = QStringLiteral("knife");
    searchPage.totalCount = 123;
    QVERIFY2(repository.savePage(searchPage, searchPage.fetchedAt.addSecs(1800), &error),
             qPrintable(error));
    const QVector<MarketScopeSnapshot> afterSearch =
        repository.latestScopeSnapshots({QStringLiteral("appid:730")});
    QCOMPARE(afterSearch.first().totalCount, live.totalCount);
}

void TestFullMarket::migrationUpgradesExistingSingleKeyItemsTable() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("upgrade.db"));
    const QString setupConnection = QStringLiteral("full_market_upgrade_setup");
    {
        QSqlDatabase setup = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                       setupConnection);
        setup.setDatabaseName(path);
        QVERIFY(setup.open());
        QSqlQuery query(setup);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE schema_migrations(version INTEGER PRIMARY KEY, applied_at DATETIME)")));
        for (int version = 1; version <= 5; ++version) {
            QVERIFY(query.exec(QStringLiteral(
                "INSERT INTO schema_migrations(version,applied_at) VALUES(%1,datetime('now'))")
                                   .arg(version)));
        }
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE items(market_hash_name TEXT PRIMARY KEY,appid INTEGER NOT NULL,"
            "name TEXT NOT NULL,icon_url TEXT,item_type TEXT,rarity TEXT,"
            "first_seen_at DATETIME NOT NULL,updated_at DATETIME NOT NULL)")));
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO items VALUES('Legacy Item',730,'Legacy Item',NULL,NULL,NULL,"
            "datetime('now'),datetime('now'))")));
        setup.close();
    }
    QSqlDatabase::removeDatabase(setupConnection);
    const QString preMigrationBackup = directory.filePath(QStringLiteral("upgrade-v5.backup"));
    QVERIFY(QFile::copy(path, preMigrationBackup));

    DatabaseManager manager(path);
    QVERIFY(manager.open());
    QSqlDatabase upgraded = manager.database();
    QSqlQuery verify(upgraded);
    QVERIFY(verify.exec(QStringLiteral(
        "SELECT appid,localized_name FROM items WHERE market_hash_name='Legacy Item'")));
    QVERIFY(verify.next());
    QCOMPARE(verify.value(0).toInt(), 730);
    QVERIFY(verify.value(1).isNull());
    QVERIFY(verify.exec(QStringLiteral(
        "SELECT COUNT(*) FROM schema_migrations WHERE version=6")));
    QVERIFY(verify.next());
    QCOMPARE(verify.value(0).toInt(), 1);
    upgraded = QSqlDatabase();
    manager.close();

    // Recovery proof: a pre-migration copy restores the v5 schema and its user data.
    QVERIFY(QFile::remove(path));
    QVERIFY(QFile::copy(preMigrationBackup, path));
    const QString recoveryConnection = QStringLiteral("full_market_upgrade_recovery");
    {
        QSqlDatabase recovered = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                           recoveryConnection);
        recovered.setDatabaseName(path);
        QVERIFY(recovered.open());
        QSqlQuery recovery(recovered);
        QVERIFY(recovery.exec(QStringLiteral("SELECT MAX(version) FROM schema_migrations")));
        QVERIFY(recovery.next());
        QCOMPARE(recovery.value(0).toInt(), 5);
        QVERIFY(recovery.exec(QStringLiteral(
            "SELECT appid,name FROM items WHERE market_hash_name='Legacy Item'")));
        QVERIFY(recovery.next());
        QCOMPARE(recovery.value(0).toInt(), 730);
        QCOMPARE(recovery.value(1).toString(), QStringLiteral("Legacy Item"));
        recovered.close();
    }
    QSqlDatabase::removeDatabase(recoveryConnection);
}
