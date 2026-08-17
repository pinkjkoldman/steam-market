#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

enum class CatalogSort {
    kPopular,
    kPriceAsc,
    kPriceDesc,
    kNameAsc,
};

enum class DataOrigin {
    kSteamLive,
    kSteamCached,
};

struct MarketCatalogQuery {
    QString query;
    std::optional<int> appid;
    QStringList categoryFilters;
    CatalogSort sort = CatalogSort::kPopular;
    int offset = 0;
    int limit = 10;
    QString currency = QStringLiteral("CNY");
    QString language = QStringLiteral("schinese");
};

struct MarketCatalogItem {
    QString marketHashName;
    int appid = 0;
    QString localizedName;
    QString typeText;
    QString iconUrl;
    qint64 lowestSellMinor = 0;
    qint64 sellListings = 0;
};

struct MarketCatalogPage {
    MarketCatalogQuery query;
    int offset = 0;
    int pageSize = 0;
    qint64 totalCount = 0;
    QVector<MarketCatalogItem> items;
    QDateTime fetchedAt;
    DataOrigin origin = DataOrigin::kSteamLive;
    bool stale = false;
    QString sourceLabel = QStringLiteral("steam_community_market_public_page");
};

struct MarketScopeSnapshot {
    QString scopeKey;
    qint64 totalCount = 0;
    QDateTime fetchedAt;
    DataOrigin origin = DataOrigin::kSteamLive;
};

enum class MarketCatalogErrorCode {
    kNone,
    kRateLimited,
    kSchemaChanged,
    kPageOutOfRange,
    kOfflineNoCache,
    kCurrencyUnsupported,
};

struct MarketCatalogError {
    MarketCatalogErrorCode code = MarketCatalogErrorCode::kNone;
    QString messageKey;
    QString detail;
    bool recoverable = false;
    std::optional<qint64> retryAfterMs;

    bool isOk() const { return code == MarketCatalogErrorCode::kNone; }
    QString stableCode() const;

    static MarketCatalogError ok() { return {}; }
    static MarketCatalogError make(MarketCatalogErrorCode code, const QString &messageKey,
                                   bool recoverable, const QString &detail = QString(),
                                   std::optional<qint64> retryAfterMs = std::nullopt) {
        MarketCatalogError error;
        error.code = code;
        error.messageKey = messageKey;
        error.detail = detail;
        error.recoverable = recoverable;
        error.retryAfterMs = retryAfterMs;
        return error;
    }
};

inline QString MarketCatalogError::stableCode() const {
    switch (code) {
    case MarketCatalogErrorCode::kRateLimited:
        return QStringLiteral("MARKET_RATE_LIMITED");
    case MarketCatalogErrorCode::kSchemaChanged:
        return QStringLiteral("MARKET_SCHEMA_CHANGED");
    case MarketCatalogErrorCode::kPageOutOfRange:
        return QStringLiteral("MARKET_PAGE_OUT_OF_RANGE");
    case MarketCatalogErrorCode::kOfflineNoCache:
        return QStringLiteral("MARKET_OFFLINE_NO_CACHE");
    case MarketCatalogErrorCode::kCurrencyUnsupported:
        return QStringLiteral("MARKET_CURRENCY_UNSUPPORTED");
    case MarketCatalogErrorCode::kNone:
        return QString();
    }
    return QStringLiteral("MARKET_SCHEMA_CHANGED");
}

Q_DECLARE_METATYPE(MarketCatalogPage)
Q_DECLARE_METATYPE(MarketCatalogError)
Q_DECLARE_METATYPE(QVector<MarketScopeSnapshot>)
