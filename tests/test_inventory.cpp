#include "test_inventory.h"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QtTest>

#include "core/services/MultiSellHandoffService.h"
#include "core/services/PricingDraftService.h"
#include "core/services/SteamSessionService.h"
#include "data/repositories/InventoryRepository.h"
#include "network/IWebSessionHost.h"
#include "network/InventoryParser.h"
#include "test_helpers.h"

namespace {
class FakeWebSessionHost final : public IWebSessionHost {
public:
    void showLogin() override {}
    void openOfficialUrl(const QUrl &) override {}
    void prepareFreshLoginSession(std::function<void(FreshSessionResult)> completion) override {
        if (completion) completion(FreshSessionResult::Ready);
    }

    void clearSession() override {}

    void publishCookies(const QString &steamId, const QList<QNetworkCookie> &cookies) {
        emit sessionCookiesReady(steamId, cookies);
    }
};

QByteArray inventoryPayload() {
    return R"json({
        "success": 1,
        "total_inventory_count": 3,
        "more_items": true,
        "last_assetid": "9003",
        "assets": [
            {"assetid":"9001","classid":"100","instanceid":"0","amount":"1"},
            {"assetid":"9002","classid":"100","instanceid":"0","amount":"1"},
            {"assetid":"9003","classid":"200","instanceid":"0","amount":"1"}
        ],
        "descriptions": [
            {"classid":"100","instanceid":"0","market_hash_name":"Card A","name":"卡牌 A","type":"交易卡牌","marketable":1,"tradable":1,
             "tags":[{"category":"item_class","internal_name":"item_class_2"}]},
            {"classid":"200","instanceid":"0","market_hash_name":"Emote A","name":"表情 A","type":"表情","marketable":1,"tradable":1,
             "tags":[{"category":"item_class","internal_name":"item_class_4"}]}
        ]
    })json";
}
}

void TestInventory::parserJoinsAssetsAndDescriptions() {
    AppError error;
    const InventoryPage page = InventoryParser::parse(inventoryPayload(), 753, &error);
    QVERIFY(error.isOk());
    QCOMPARE(page.items.size(), 3);
    QCOMPARE(page.items.at(0).description.marketHashName, QStringLiteral("Card A"));
    QCOMPARE(page.items.at(1).description.category, QStringLiteral("trading_card"));
    QCOMPARE(page.items.at(2).description.category, QStringLiteral("emoticon"));
    QVERIFY(page.hasMore);
    QCOMPARE(page.lastAssetId, QStringLiteral("9003"));
}

void TestInventory::parserRejectsInvalidPayload() {
    AppError error;
    const InventoryPage page = InventoryParser::parse(QByteArray("not-json"), 753, &error);
    QVERIFY(!error.isOk());
    QVERIFY(page.items.isEmpty());
}

void TestInventory::repositoryCompletesAtomicSnapshot() {
    TestDb fixture;
    QVERIFY(fixture.opened);
    InventoryRepository repository(fixture.handle);
    AppError parseError;
    InventoryPage page = InventoryParser::parse(inventoryPayload(), 753, &parseError);
    QVERIFY(parseError.isOk());
    page.hasMore = false;
    QString error;
    const QString syncId = QStringLiteral("sync-test-1");
    QVERIFY(repository.beginSync(syncId, QStringLiteral("76561198000000000"), 753,
                                 QStringLiteral("6"), &error));
    QVERIFY(repository.savePage(syncId, QStringLiteral("76561198000000000"), 753,
                                QStringLiteral("6"), page, 1, 3, &error));
    QVERIFY(repository.completeSync(syncId, QStringLiteral("76561198000000000"), 753,
                                    QStringLiteral("6"), 1, 3, &error));
    const QVector<InventoryGroup> groups = repository.groups(
        QStringLiteral("76561198000000000"), 753, QStringLiteral("6"));
    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups.at(0).marketHashName, QStringLiteral("Emote A"));
    QCOMPARE(groups.at(1).marketHashName, QStringLiteral("Card A"));
    QCOMPARE(groups.at(1).inventoryQuantity, 2);
    QCOMPARE(groups.at(1).assetIds.size(), 2);
}

void TestInventory::fixedPriceDraftUsesMinorUnits() {
    InventoryGroup group;
    group.appid = 753;
    group.contextId = QStringLiteral("6");
    group.marketHashName = QStringLiteral("Card A");
    group.displayName = QStringLiteral("卡牌 A");
    group.inventoryQuantity = 2;
    group.selectedQuantity = 2;
    group.assetIds = {QStringLiteral("9001"), QStringLiteral("9002")};
    PricingDraftService service;
    QString error;
    const QVector<ListingDraftLine> lines = service.createFixedPriceDraft({group}, 115, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(lines.size(), 1);
    QCOMPARE(lines.first().buyerPaysMinor, 115);
    QCOMPARE(lines.first().feeMinor, 15);
    QCOMPARE(lines.first().sellerReceivesMinor, 100);
}

void TestInventory::handoffSplitsLargeSelections() {
    QVector<ListingDraftLine> lines;
    for (int index = 0; index < 41; ++index) {
        ListingDraftLine line;
        line.group.appid = 753;
        line.group.contextId = QStringLiteral("6");
        line.group.marketHashName = QStringLiteral("Card %1").arg(index);
        line.group.selectedQuantity = 1;
        lines.append(line);
    }
    MultiSellHandoffService service;
    QString error;
    const QVector<HandoffBatch> batches = service.createBatches(lines, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(batches.size(), 2);
    QCOMPARE(batches.first().groupCount, 40);
    QCOMPARE(batches.last().groupCount, 1);
    QCOMPARE(batches.first().officialUrl.host(), QStringLiteral("steamcommunity.com"));
}

void TestInventory::sessionDerivesSteamIdFromLoginCookie() {
    FakeWebSessionHost host;
    QNetworkAccessManager network;
    auto *cookieJar = new QNetworkCookieJar(&network);
    network.setCookieJar(cookieJar);
    SteamSessionService service(&host, &network);
    QVERIFY(service.beginOfficialLogin());
    QSignalSpy changed(&service, &SteamSessionService::sessionChanged);

    QNetworkCookie loginCookie(
        QByteArray("steamLoginSecure"),
        QByteArray("76561198000000000%7C%7Cfixture-token"));
    loginCookie.setDomain(QStringLiteral(".steamcommunity.com"));
    loginCookie.setPath(QStringLiteral("/"));
    loginCookie.setSecure(true);
    host.publishCookies(QString(), {loginCookie});

    QCOMPARE(changed.count(), 1);
    QCOMPARE(service.steamId(), QStringLiteral("76561198000000000"));
    QVERIFY(service.hasSession());
    QVERIFY(!cookieJar->cookiesForUrl(
                 QUrl(QStringLiteral("https://steamcommunity.com/inventory/"))).isEmpty());
    service.logout();
    QVERIFY(!service.hasSession());
    QVERIFY(cookieJar->cookiesForUrl(QUrl(QStringLiteral("https://steamcommunity.com"))).isEmpty());
}
