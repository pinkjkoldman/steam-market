#include "test_steam_orderbook_parser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QtTest>

#include "network/SteamMarketClient.h"
#include "network/SteamOrderbookParser.h"

namespace {
class PendingReply final : public QNetworkReply {
public:
    PendingReply(QNetworkAccessManager::Operation operation,
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

class RequestCapture final : public QNetworkAccessManager {
public:
    QNetworkRequest lastRequest;

protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request,
                                 QIODevice *) override {
        lastRequest = request;
        return new PendingReply(operation, request, this);
    }
};
}  // namespace

void TestSteamOrderbookParser::parsesQueryActionPayload() {
    const QByteArray body = R"json({
        "data": {
            "success": true,
            "data": {
                "amtMaxBuyOrder": 4066,
                "amtMinSellOrder": 4068,
                "eCurrency": 1,
                "cBuyOrders": 25,
                "cSellOrders": 8,
                "rgCompactBuyOrders": [4066, 10, 4065, 18],
                "rgCompactSellOrders": [4068, 3, 4070, 5]
            }
        }
    })json";

    const SteamOrderbookParseResult result = SteamOrderbookParser::parseQueryAction(
        body, QStringLiteral("AK-47 | Redline (Field-Tested)"));
    QVERIFY2(result.error.isOk(), qPrintable(result.error.message));
    QCOMPARE(result.orderbook.marketHashName,
             QStringLiteral("AK-47 | Redline (Field-Tested)"));
    QCOMPARE(result.orderbook.steamCurrencyId, 1);
    QCOMPARE(result.orderbook.highestBuy, 40.66);
    QCOMPARE(result.orderbook.lowestSell, 40.68);
    QCOMPARE(result.orderbook.buyOrders.size(), 2);
    QCOMPARE(result.orderbook.buyOrders.first().count, 10);
    QCOMPARE(result.orderbook.sellOrders.last().price, 40.70);
}

void TestSteamOrderbookParser::rejectsLegacyAndMalformedShapes() {
    const SteamOrderbookParseResult legacy = SteamOrderbookParser::parseQueryAction(
        R"json({"success":true,"buy_order_graph":[]})json", QStringLiteral("Item"));
    QCOMPARE(legacy.error.code, ErrorCode::kDataSourceUnavailable);

    const SteamOrderbookParseResult oddCompact = SteamOrderbookParser::parseQueryAction(
        R"json({"data":{"success":true,"data":{"amtMaxBuyOrder":1,
        "amtMinSellOrder":2,"eCurrency":1,"rgCompactBuyOrders":[1],
        "rgCompactSellOrders":[]}}})json", QStringLiteral("Item"));
    QCOMPARE(oddCompact.error.code, ErrorCode::kSourceInvalid);
}

void TestSteamOrderbookParser::clientUsesOfficialQueryActionProtocol() {
    RequestCapture network;
    SteamMarketClient client(&network);
    client.setRequestIntervalMs(0);
    client.fetchOrderbook(QStringLiteral("AK-47 | Redline (Field-Tested)"), 730,
                          [](const Orderbook &, const AppError &) {});

    QCOMPARE(network.lastRequest.url().scheme(), QStringLiteral("https"));
    QCOMPARE(network.lastRequest.url().host(), QStringLiteral("steamcommunity.com"));
    QCOMPARE(network.lastRequest.url().path(), QStringLiteral("/market/orderbook"));
    QCOMPARE(network.lastRequest.rawHeader("x-valve-request-type"),
             QByteArray("queryAction"));

    const QUrlQuery query(network.lastRequest.url());
    QCOMPARE(query.queryItemValue(QStringLiteral("q")), QStringLiteral("Load"));
    const QJsonDocument parameters = QJsonDocument::fromJson(
        query.queryItemValue(QStringLiteral("qp")).toUtf8());
    QVERIFY(parameters.isArray());
    QCOMPARE(parameters.array().size(), 2);
    QCOMPARE(parameters.array().at(0).toInt(), 730);
    QCOMPARE(parameters.array().at(1).toString(),
             QStringLiteral("AK-47 | Redline (Field-Tested)"));
}
