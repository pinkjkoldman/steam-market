#include "test_inventory.h"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QtTest>

#include "core/services/MultiSellHandoffService.h"
#include "core/services/PricingDraftService.h"
#include "core/services/SteamSessionService.h"
#include "core/services/TradeHandoffService.h"
#include "data/repositories/InventoryRepository.h"
#include "network/IWebSessionHost.h"
#include "network/InventoryParser.h"
#include "network/SteamInventoryClient.h"
#include "test_helpers.h"
#include "ui/widgets/WorkbenchTheme.h"

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

class StubNetworkReply final : public QNetworkReply {
public:
    StubNetworkReply(QNetworkAccessManager::Operation operation,
                     const QNetworkRequest &request, QObject *parent)
        : QNetworkReply(parent) {
        setOperation(operation);
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
    }

    void abort() override {}

protected:
    qint64 readData(char *, qint64) override { return -1; }
};

class CapturingNetworkAccessManager final : public QNetworkAccessManager {
public:
    QUrl lastUrl;

protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request,
                                 QIODevice *) override {
        lastUrl = request.url();
        return new StubNetworkReply(operation, request, this);
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
            {"classid":"200","instanceid":"0","market_hash_name":"Emote A","name":"表情 A","type":"表情","marketable":0,"tradable":1,
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
    const QVector<InventoryGroup> marketableGroups = repository.groups(
        QStringLiteral("76561198000000000"), 753, QStringLiteral("6"));
    QCOMPARE(marketableGroups.size(), 1);
    QCOMPARE(marketableGroups.at(0).marketHashName, QStringLiteral("Card A"));
    QCOMPARE(marketableGroups.at(0).inventoryQuantity, 2);
    QCOMPARE(marketableGroups.at(0).assetIds.size(), 2);
    QVERIFY(marketableGroups.at(0).marketable);
    QVERIFY(marketableGroups.at(0).tradable);

    const QVector<InventoryGroup> allGroups = repository.groups(
        QStringLiteral("76561198000000000"), 753, QStringLiteral("6"), false);
    QCOMPARE(allGroups.size(), 2);
    QCOMPARE(allGroups.at(0).marketHashName, QStringLiteral("Emote A"));
    QVERIFY(!allGroups.at(0).marketable);
    QVERIFY(allGroups.at(0).tradable);
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

void TestInventory::singleItemHandoffUsesOfficialSteamPages() {
    TradeHandoffService service;
    InventoryGroup communityItem;
    communityItem.appid = 753;
    communityItem.contextId = QStringLiteral("6");
    communityItem.marketHashName = QStringLiteral("Card A");
    communityItem.assetIds = {QStringLiteral("9001")};
    communityItem.marketable = true;
    QString error;
    const QUrl communityUrl = service.createSingleListingUrl(
        communityItem, QStringLiteral("76561198000000000"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(communityUrl.host(), QStringLiteral("steamcommunity.com"));
    QCOMPARE(communityUrl.path(), QStringLiteral("/market/multisell"));
    QCOMPARE(QUrlQuery(communityUrl).queryItemValue(QStringLiteral("items[]")),
             QStringLiteral("Card A"));

    InventoryGroup uniqueItem = communityItem;
    uniqueItem.appid = 730;
    uniqueItem.contextId = QStringLiteral("2");
    const QUrl inventoryUrl = service.createSingleListingUrl(
        uniqueItem, QStringLiteral("76561198000000000"), &error);
    QCOMPARE(inventoryUrl.path(),
             QStringLiteral("/profiles/76561198000000000/inventory/"));
    QCOMPARE(inventoryUrl.fragment(), QStringLiteral("730_2_9001"));
}

void TestInventory::tradeOfferHandoffValidatesPartner() {
    TradeHandoffService service;
    QString error;
    const QUrl friendUrl = service.createTradeOfferUrl(
        QStringLiteral("76561198000000000"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(friendUrl.host(), QStringLiteral("steamcommunity.com"));
    QCOMPARE(QUrlQuery(friendUrl).queryItemValue(QStringLiteral("partner")),
             QStringLiteral("39734272"));

    const QUrl tokenUrl = service.createTradeOfferUrl(
        QStringLiteral("https://steamcommunity.com/tradeoffer/new/?partner=39734272&token=Ab_12-c"),
        &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(QUrlQuery(tokenUrl).queryItemValue(QStringLiteral("token")),
             QStringLiteral("Ab_12-c"));

    QVERIFY(service.createTradeOfferUrl(
                QStringLiteral("https://steamcommunity.com.evil.test/tradeoffer/new/?partner=1"),
                &error)
                .isEmpty());
    QVERIFY(!error.isEmpty());

    QVERIFY(service.createTradeOfferUrl(
                QStringLiteral("https://steamcommunity.com/tradeoffer/new/?partner=1&redirect=evil"),
                &error)
                .isEmpty());
    QVERIFY(!error.isEmpty());
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

void TestInventory::inventoryRequestUsesSteamSupportedPageSize() {
    CapturingNetworkAccessManager network;
    SteamInventoryClient client(&network);
    client.fetchPage(QStringLiteral("76561198049216736"), 753, QStringLiteral("6"), {},
                     [](const InventoryPage &, const AppError &) {});

    const QUrlQuery query(network.lastUrl);
    QCOMPARE(query.queryItemValue(QStringLiteral("count")), QStringLiteral("2000"));
}

void TestInventory::dropdownThemeStylesPopupItems() {
    const QString style = WorkbenchTheme::styleSheet();
    QVERIFY2(style.contains(QStringLiteral("QComboBox QAbstractItemView {")),
             "Combo popup needs an explicit dark background and foreground");
    QVERIFY2(style.contains(QStringLiteral("QComboBox QAbstractItemView::item:selected")),
             "Combo popup needs a readable selected-item state");
    QVERIFY2(style.contains(QStringLiteral("QComboBox::drop-down")),
             "Combo drop-down button needs an explicit visible boundary");
    QVERIFY2(style.contains(QStringLiteral("QComboBox::down-arrow"))
                 && style.contains(QStringLiteral(":/ui/chevron-down.svg")),
             "Combo needs an explicit high-contrast arrow asset");
}
