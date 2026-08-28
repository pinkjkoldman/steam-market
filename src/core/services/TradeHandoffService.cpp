#include "core/services/TradeHandoffService.h"

#include <QRegularExpression>
#include <QUrlQuery>

namespace {
constexpr quint64 kSteamId64Base = 76561197960265728ULL;
constexpr quint64 kMaxAccountId = 0xFFFFFFFFULL;

bool digitsOnly(const QString &value, int minimumLength, int maximumLength) {
    if (value.size() < minimumLength || value.size() > maximumLength) return false;
    for (const QChar character : value) {
        if (!character.isDigit()) return false;
    }
    return true;
}

void fail(QString *error, const QString &message) {
    if (error) *error = message;
}

QUrl canonicalTradeUrl(const QString &partner, const QString &token = QString()) {
    QUrl url(QStringLiteral("https://steamcommunity.com/tradeoffer/new/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("partner"), partner);
    if (!token.isEmpty()) query.addQueryItem(QStringLiteral("token"), token);
    url.setQuery(query);
    return url;
}
}  // namespace

QUrl TradeHandoffService::createSingleListingUrl(const InventoryGroup &group,
                                                  const QString &sellerSteamId64,
                                                  QString *error) const {
    if (!group.marketable) {
        fail(error, QStringLiteral("Steam 将该物品标记为不可在市场出售"));
        return {};
    }
    if (group.appid <= 0 || !digitsOnly(group.contextId, 1, 20)
        || group.assetIds.isEmpty() || !digitsOnly(group.assetIds.first(), 1, 32)
        || !digitsOnly(sellerSteamId64, 17, 17)) {
        fail(error, QStringLiteral("物品资产标识不完整，请重新同步库存"));
        return {};
    }

    QUrl url;
    if (group.appid == 753 && !group.marketHashName.trimmed().isEmpty()
        && group.marketHashName.size() <= 256) {
        url = QUrl(QStringLiteral("https://steamcommunity.com/market/multisell"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("appid"), QString::number(group.appid));
        query.addQueryItem(QStringLiteral("contextid"), group.contextId);
        query.addQueryItem(QStringLiteral("items[]"), group.marketHashName);
        url.setQuery(query);
    } else {
        url = QUrl(QStringLiteral("https://steamcommunity.com/profiles/%1/inventory/")
                       .arg(sellerSteamId64));
        url.setFragment(QStringLiteral("%1_%2_%3")
                            .arg(group.appid)
                            .arg(group.contextId, group.assetIds.first()));
    }
    if (error) error->clear();
    return url;
}

QUrl TradeHandoffService::createTradeOfferUrl(const QString &partnerInput,
                                               QString *error) const {
    const QString input = partnerInput.trimmed();
    if (input.isEmpty()) {
        if (error) error->clear();
        return QUrl(QStringLiteral("https://steamcommunity.com/tradeoffer/new/"));
    }

    if (digitsOnly(input, 17, 17)) {
        bool ok = false;
        const quint64 steamId64 = input.toULongLong(&ok);
        if (!ok || steamId64 < kSteamId64Base
            || steamId64 - kSteamId64Base > kMaxAccountId) {
            fail(error, QStringLiteral("对方 SteamID64 无效"));
            return {};
        }
        if (error) error->clear();
        return canonicalTradeUrl(QString::number(steamId64 - kSteamId64Base));
    }

    const QUrl supplied(input, QUrl::StrictMode);
    const QString path = supplied.path().endsWith(QLatin1Char('/'))
                             ? supplied.path()
                             : supplied.path() + QLatin1Char('/');
    if (!supplied.isValid() || supplied.scheme() != QLatin1String("https")
        || supplied.host().compare(QStringLiteral("steamcommunity.com"),
                                   Qt::CaseInsensitive) != 0
        || (supplied.port() != -1 && supplied.port() != 443)
        || !supplied.userInfo().isEmpty() || !supplied.fragment().isEmpty()
        || path != QLatin1String("/tradeoffer/new/")) {
        fail(error, QStringLiteral("请输入 17 位 SteamID64 或 Steam 官方交易报价链接"));
        return {};
    }

    const QUrlQuery query(supplied);
    int partnerCount = 0;
    int tokenCount = 0;
    for (const auto &item : query.queryItems(QUrl::FullyDecoded)) {
        if (item.first == QLatin1String("partner")) {
            ++partnerCount;
        } else if (item.first == QLatin1String("token")) {
            ++tokenCount;
        } else {
            fail(error, QStringLiteral("交易报价链接包含不支持的参数"));
            return {};
        }
    }
    if (partnerCount != 1 || tokenCount > 1) {
        fail(error, QStringLiteral("交易报价链接的参数数量无效"));
        return {};
    }
    const QString partner = query.queryItemValue(QStringLiteral("partner"));
    const QString token = query.queryItemValue(QStringLiteral("token"));
    bool accountOk = false;
    const quint64 accountId = partner.toULongLong(&accountOk);
    static const QRegularExpression tokenPattern(QStringLiteral("^[A-Za-z0-9_-]{1,64}$"));
    if (!accountOk || !digitsOnly(partner, 1, 10) || accountId > kMaxAccountId
        || (!token.isEmpty() && !tokenPattern.match(token).hasMatch())) {
        fail(error, QStringLiteral("交易报价链接的 partner 或 token 无效"));
        return {};
    }
    if (error) error->clear();
    return canonicalTradeUrl(partner, token);
}
