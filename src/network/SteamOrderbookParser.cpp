#include "network/SteamOrderbookParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtMath>

namespace {
constexpr int kMaxCompactValues = 20000;

bool safeInteger(const QJsonValue &value, qint64 minimum, qint64 maximum,
                 qint64 *output) {
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!qIsFinite(number) || qFloor(number) != number
        || number < static_cast<double>(minimum)
        || number > static_cast<double>(maximum)) {
        return false;
    }
    if (output) *output = static_cast<qint64>(number);
    return true;
}

bool parseCompactOrders(const QJsonValue &value, QVector<OrderbookEntry> *entries) {
    if (!entries || !value.isArray()) return false;
    const QJsonArray compact = value.toArray();
    if (compact.size() % 2 != 0 || compact.size() > kMaxCompactValues) return false;
    entries->reserve(compact.size() / 2);
    for (qsizetype index = 0; index < compact.size(); index += 2) {
        qint64 priceMinor = 0;
        qint64 quantity = 0;
        if (!safeInteger(compact.at(index), 1, 1000000000000LL, &priceMinor)
            || !safeInteger(compact.at(index + 1), 1, 1000000000LL, &quantity)) {
            return false;
        }
        entries->append({static_cast<double>(priceMinor) / 100.0,
                         static_cast<int>(quantity)});
    }
    return true;
}
}  // namespace

SteamOrderbookParseResult SteamOrderbookParser::parseQueryAction(
    const QByteArray &body, const QString &marketHashName) {
    SteamOrderbookParseResult result;
    result.orderbook.marketHashName = marketHashName;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.error = AppError::make(ErrorCode::kSourceInvalid,
                                      QStringLiteral("Steam 盘口响应不是有效 JSON"));
        return result;
    }

    const QJsonObject action = document.object().value(QStringLiteral("data")).toObject();
    const QJsonObject data = action.value(QStringLiteral("data")).toObject();
    if (!action.value(QStringLiteral("success")).toBool(false) || data.isEmpty()) {
        result.error = AppError::make(ErrorCode::kDataSourceUnavailable,
                                      QStringLiteral("Steam 未返回可用盘口"));
        return result;
    }

    qint64 highestBuyMinor = 0;
    qint64 lowestSellMinor = 0;
    qint64 currencyId = 0;
    if (!safeInteger(data.value(QStringLiteral("amtMaxBuyOrder")), 0,
                     1000000000000LL, &highestBuyMinor)
        || !safeInteger(data.value(QStringLiteral("amtMinSellOrder")), 0,
                        1000000000000LL, &lowestSellMinor)
        || !safeInteger(data.value(QStringLiteral("eCurrency")), 1, 255, &currencyId)
        || !parseCompactOrders(data.value(QStringLiteral("rgCompactBuyOrders")),
                               &result.orderbook.buyOrders)
        || !parseCompactOrders(data.value(QStringLiteral("rgCompactSellOrders")),
                               &result.orderbook.sellOrders)) {
        result.orderbook = {};
        result.orderbook.marketHashName = marketHashName;
        result.error = AppError::make(ErrorCode::kSourceInvalid,
                                      QStringLiteral("Steam 盘口响应结构已变化"));
        return result;
    }

    result.orderbook.highestBuy = highestBuyMinor > 0
                                      ? static_cast<double>(highestBuyMinor) / 100.0
                                      : -1.0;
    result.orderbook.lowestSell = lowestSellMinor > 0
                                      ? static_cast<double>(lowestSellMinor) / 100.0
                                      : -1.0;
    result.orderbook.steamCurrencyId = static_cast<int>(currencyId);
    result.orderbook.fetchedAt = QDateTime::currentDateTimeUtc();
    result.error = AppError::ok();
    return result;
}
