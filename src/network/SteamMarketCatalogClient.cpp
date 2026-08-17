#include "network/SteamMarketCatalogClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QUrlQuery>
#include <cmath>
#include <limits>

#include "utils/Currency.h"

namespace {
constexpr qint64 kMaximumResponseBytes = 2 * 1024 * 1024;
constexpr qint64 kMaximumTotalCount = 10000000;
constexpr qint64 kMaximumSafeJsonInteger = 9007199254740991LL;
constexpr int kMaximumOffset = 10000000;
constexpr int kMaximumNetworkRetries = 2;

class AnonymousCookieJar final : public QNetworkCookieJar {
public:
    explicit AnonymousCookieJar(QObject *parent = nullptr) : QNetworkCookieJar(parent) {}

    bool setCookiesFromUrl(const QList<QNetworkCookie> &, const QUrl &) override {
        return false;
    }

    QList<QNetworkCookie> cookiesForUrl(const QUrl &) const override {
        return {};
    }
};

QString sortColumn(CatalogSort sort) {
    switch (sort) {
    case CatalogSort::kPriceAsc:
    case CatalogSort::kPriceDesc:
        return QStringLiteral("price");
    case CatalogSort::kNameAsc:
        return QStringLiteral("name");
    case CatalogSort::kPopular:
        return QStringLiteral("popular");
    }
    return QStringLiteral("popular");
}

QString sortDirection(CatalogSort sort) {
    return sort == CatalogSort::kPriceAsc || sort == CatalogSort::kNameAsc
               ? QStringLiteral("asc")
               : QStringLiteral("desc");
}

bool isJsonContentType(const QString &contentType) {
    const QString type = contentType.section(QLatin1Char(';'), 0, 0).trimmed().toLower();
    return type == QLatin1String("application/json")
           || type == QLatin1String("text/json")
           || type == QLatin1String("text/javascript");
}

bool boundedString(const QJsonValue &value, int maximumLength, QString *out,
                   bool allowEmpty = true) {
    if (!value.isString()) return false;
    const QString result = value.toString();
    if (result.size() > maximumLength || (!allowEmpty && result.isEmpty())) return false;
    *out = result;
    return true;
}

bool optionalBoundedString(const QJsonObject &object, const QString &key,
                           int maximumLength, QString *out) {
    const QJsonValue value = object.value(key);
    if (value.isUndefined() || value.isNull()) {
        out->clear();
        return true;
    }
    return boundedString(value, maximumLength, out);
}

bool safeNonNegativeInteger(const QJsonValue &value, qint64 maximum, qint64 *out) {
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0 || number > static_cast<double>(maximum)
        || std::floor(number) != number) {
        return false;
    }
    *out = static_cast<qint64>(number);
    return true;
}

QString safeIconUrl(const QString &raw) {
    if (raw.isEmpty()) return QString();
    QUrl url(raw);
    if (url.isRelative() && !raw.startsWith(QStringLiteral("//"))) {
        if (raw.size() > 1024 || raw.contains(QLatin1Char('/'))) return QString();
        url = QUrl(QStringLiteral("https://community.cloudflare.steamstatic.com/economy/image/")
                   + raw);
    } else if (raw.startsWith(QStringLiteral("//"))) {
        url = QUrl(QStringLiteral("https:") + raw);
    }
    const QString host = url.host().toLower();
    const bool allowedHost = host == QLatin1String("steamcommunity-a.akamaihd.net")
                             || host == QLatin1String("community.akamai.steamstatic.com")
                             || host == QLatin1String("community.cloudflare.steamstatic.com");
    if (!url.isValid() || url.scheme() != QLatin1String("https") || !allowedHost
        || url.userInfo().size() > 0 || url.toString().size() > 2048) {
        return QString();
    }
    return url.toString(QUrl::FullyEncoded);
}

MarketCatalogError schemaError(const QString &detail) {
    return MarketCatalogError::make(MarketCatalogErrorCode::kSchemaChanged,
                                    QStringLiteral("market.schema_changed"), false, detail);
}
}  // namespace

SteamMarketCatalogClient::SteamMarketCatalogClient(QObject *parent) : QObject(parent) {
    m_network = new QNetworkAccessManager(this);
    m_network->setCookieJar(new AnonymousCookieJar(m_network));
    m_dispatchTimer.setSingleShot(true);
    connect(&m_dispatchTimer, &QTimer::timeout, this, &SteamMarketCatalogClient::dispatch);
}

SteamMarketCatalogClient::~SteamMarketCatalogClient() = default;

MarketCatalogError SteamMarketCatalogClient::validateQuery(const MarketCatalogQuery &query) {
    if (query.query.size() > 128 || (query.appid.has_value() && query.appid.value() <= 0)
        || query.offset < 0 || query.offset > kMaximumOffset || query.limit < 1
        || query.limit > 10) {
        return schemaError(QStringLiteral("invalid catalog query bounds"));
    }
    if (query.currency != QLatin1String("CNY") && query.currency != QLatin1String("USD")) {
        return MarketCatalogError::make(MarketCatalogErrorCode::kCurrencyUnsupported,
                                        QStringLiteral("market.currency_unsupported"), false);
    }
    if (query.language != QLatin1String("schinese")
        && query.language != QLatin1String("english")) {
        return schemaError(QStringLiteral("unsupported language"));
    }
    if (query.categoryFilters.size() > 32) {
        return schemaError(QStringLiteral("too many category filters"));
    }
    for (const QString &filter : query.categoryFilters) {
        if (filter.isEmpty() || filter.size() > 128) return schemaError(QStringLiteral("invalid category filter"));
    }
    return MarketCatalogError::ok();
}

qint64 SteamMarketCatalogClient::retryAfterDelayMs(const QByteArray &value) {
    constexpr qint64 maximumDelayMs = 24 * 60 * 60 * 1000;
    bool secondsOk = false;
    const qint64 seconds = value.trimmed().toLongLong(&secondsOk);
    if (!secondsOk || seconds < 0) return 30000;
    if (seconds >= maximumDelayMs / 1000) return maximumDelayMs;
    return seconds * 1000;
}

QUrl SteamMarketCatalogClient::requestUrl(const MarketCatalogQuery &query) {
    QUrl url(QStringLiteral("https://steamcommunity.com/market/search/render/"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("query"), query.query);
    urlQuery.addQueryItem(QStringLiteral("start"), QString::number(query.offset));
    urlQuery.addQueryItem(QStringLiteral("count"), QStringLiteral("10"));
    urlQuery.addQueryItem(QStringLiteral("search_descriptions"), QStringLiteral("0"));
    urlQuery.addQueryItem(QStringLiteral("sort_column"), sortColumn(query.sort));
    urlQuery.addQueryItem(QStringLiteral("sort_dir"), sortDirection(query.sort));
    urlQuery.addQueryItem(QStringLiteral("norender"), QStringLiteral("1"));
    urlQuery.addQueryItem(QStringLiteral("currency"),
                          QString::number(Currency::steamId(query.currency)));
    urlQuery.addQueryItem(QStringLiteral("l"), query.language);
    if (query.appid.has_value()) {
        urlQuery.addQueryItem(QStringLiteral("appid"), QString::number(query.appid.value()));
    }
    for (const QString &filter : query.categoryFilters) {
        if (!filter.isEmpty() && filter.size() <= 128) {
            urlQuery.addQueryItem(QStringLiteral("category[]"), filter);
        }
    }
    url.setQuery(urlQuery);
    return url;
}

bool SteamMarketCatalogClient::parseResponse(const QByteArray &payload,
                                             const MarketCatalogQuery &query,
                                             const QDateTime &fetchedAt,
                                             MarketCatalogPage *page,
                                             MarketCatalogError *error) {
    if (!page || !error) return false;
    *page = {};
    *error = MarketCatalogError::ok();
    if (payload.size() > kMaximumResponseBytes) {
        *error = schemaError(QStringLiteral("catalog response exceeds 2 MiB"));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        *error = schemaError(QStringLiteral("catalog response is not a JSON object"));
        return false;
    }
    const QJsonObject root = document.object();
    if (!root.value(QStringLiteral("success")).isBool()
        || !root.value(QStringLiteral("success")).toBool()) {
        *error = schemaError(QStringLiteral("catalog success flag is absent or false"));
        return false;
    }
    qint64 start = 0;
    qint64 pageSize = 0;
    qint64 totalCount = 0;
    if (!safeNonNegativeInteger(root.value(QStringLiteral("start")), kMaximumOffset, &start)
        || !safeNonNegativeInteger(root.value(QStringLiteral("pagesize")), 100, &pageSize)
        || pageSize < 1
        || !safeNonNegativeInteger(root.value(QStringLiteral("total_count")),
                                   kMaximumTotalCount, &totalCount)
        || start != query.offset || !root.value(QStringLiteral("results")).isArray()) {
        *error = schemaError(QStringLiteral("catalog page metadata is invalid"));
        return false;
    }
    const QJsonArray results = root.value(QStringLiteral("results")).toArray();
    if (results.size() > pageSize) {
        *error = schemaError(QStringLiteral("catalog result count exceeds pagesize"));
        return false;
    }
    if (query.offset >= totalCount && totalCount > 0 && !results.isEmpty()) {
        *error = MarketCatalogError::make(MarketCatalogErrorCode::kPageOutOfRange,
                                          QStringLiteral("market.page_out_of_range"), true);
        return false;
    }

    MarketCatalogPage parsed;
    parsed.query = query;
    parsed.offset = static_cast<int>(start);
    parsed.pageSize = static_cast<int>(pageSize);
    parsed.totalCount = totalCount;
    parsed.fetchedAt = fetchedAt.toUTC();
    parsed.origin = DataOrigin::kSteamLive;
    parsed.stale = false;
    parsed.items.reserve(results.size());
    for (const QJsonValue &value : results) {
        if (!value.isObject()) {
            *error = schemaError(QStringLiteral("catalog result is not an object"));
            return false;
        }
        const QJsonObject result = value.toObject();
        if (!result.value(QStringLiteral("asset_description")).isObject()) {
            *error = schemaError(QStringLiteral("asset_description is absent"));
            return false;
        }
        const QJsonObject asset = result.value(QStringLiteral("asset_description")).toObject();
        MarketCatalogItem item;
        qint64 appid = 0;
        if (!safeNonNegativeInteger(asset.value(QStringLiteral("appid")),
                                    std::numeric_limits<int>::max(), &appid)
            || appid < 1 || !boundedString(result.value(QStringLiteral("hash_name")), 512,
                                           &item.marketHashName, false)
            || !boundedString(result.value(QStringLiteral("name")), 512,
                              &item.localizedName, false)
            || !optionalBoundedString(asset, QStringLiteral("type"), 256, &item.typeText)
            || !safeNonNegativeInteger(result.value(QStringLiteral("sell_price")),
                                       kMaximumSafeJsonInteger,
                                       &item.lowestSellMinor)
            || !safeNonNegativeInteger(result.value(QStringLiteral("sell_listings")),
                                       kMaximumSafeJsonInteger,
                                       &item.sellListings)) {
            *error = schemaError(QStringLiteral("catalog item field is invalid"));
            return false;
        }
        item.appid = static_cast<int>(appid);
        QString rawIcon;
        if (!optionalBoundedString(asset, QStringLiteral("icon_url"), 2048, &rawIcon)) {
            *error = schemaError(QStringLiteral("catalog icon field is invalid"));
            return false;
        }
        item.iconUrl = safeIconUrl(rawIcon);
        parsed.items.append(item);
    }
    *page = parsed;
    return true;
}

void SteamMarketCatalogClient::fetchPage(const MarketCatalogQuery &query,
                                         MarketCatalogClientCallback callback) {
    cancelCurrent();
    const MarketCatalogError validation = validateQuery(query);
    if (!validation.isOk()) {
        if (callback) callback({}, validation);
        return;
    }
    ActiveRequest request;
    request.id = m_nextRequestId++;
    request.query = query;
    request.callback = std::move(callback);
    m_active = std::move(request);
    scheduleDispatch();
}

void SteamMarketCatalogClient::cancelCurrent() {
    m_dispatchTimer.stop();
    if (m_reply) {
        m_reply->abort();
        m_reply.clear();
    }
    m_active.reset();
}

void SteamMarketCatalogClient::setMinimumIntervalMs(qint64 intervalMs) {
    m_minimumIntervalMs = qMax<qint64>(1500, intervalMs);
}

void SteamMarketCatalogClient::scheduleDispatch(qint64 extraDelayMs) {
    if (!m_active.has_value()) return;
    qint64 delay = qMax<qint64>(0, extraDelayMs);
    if (m_hasDispatched) {
        delay = qMax(delay, m_minimumIntervalMs - m_lastDispatch.elapsed());
    }
    if (m_rateLimitedUntil.isValid()) {
        delay = qMax(delay, QDateTime::currentDateTimeUtc().msecsTo(m_rateLimitedUntil));
    }
    const qint64 boundedDelay =
        qBound(qint64(0), delay, qint64(std::numeric_limits<int>::max()));
    m_dispatchTimer.start(static_cast<int>(boundedDelay));
}

void SteamMarketCatalogClient::dispatch() {
    if (!m_active.has_value() || m_reply) return;
    QNetworkRequest request(requestUrl(m_active->query));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "SteamMarketTerminal/1.0 (+desktop; anonymous-catalog)");
    request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    m_lastDispatch.restart();
    m_hasDispatched = true;
    const quint64 requestId = m_active->id;
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this, requestId]() {
        QNetworkReply *reply = m_reply.data();
        m_reply.clear();
        if (reply) handleReply(reply, requestId);
    });
}

void SteamMarketCatalogClient::handleReply(QNetworkReply *reply, quint64 requestId) {
    reply->deleteLater();
    if (!m_active.has_value() || m_active->id != requestId) return;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirect.isValid()) {
        finish({}, schemaError(QStringLiteral("catalog endpoint redirected")));
        return;
    }
    if (status == 429) {
        const qint64 retryMs = retryAfterDelayMs(reply->rawHeader("Retry-After"));
        m_rateLimitedUntil = QDateTime::currentDateTimeUtc().addMSecs(retryMs);
        finish({}, MarketCatalogError::make(MarketCatalogErrorCode::kRateLimited,
                                             QStringLiteral("market.rate_limited"), true,
                                             QString(), retryMs));
        return;
    }
    if (status == 403) {
        finish({}, schemaError(QStringLiteral("catalog request was refused")));
        return;
    }
    if (reply->error() != QNetworkReply::NoError || status >= 500) {
        retryOrFinish(MarketCatalogError::make(
            MarketCatalogErrorCode::kOfflineNoCache,
            QStringLiteral("market.remote_unavailable"), true, reply->errorString()));
        return;
    }
    if (status < 200 || status >= 300
        || !isJsonContentType(reply->header(QNetworkRequest::ContentTypeHeader).toString())) {
        finish({}, schemaError(QStringLiteral("catalog status or content type is invalid")));
        return;
    }
    const QByteArray payload = reply->read(kMaximumResponseBytes + 1);
    MarketCatalogPage page;
    MarketCatalogError error;
    if (!parseResponse(payload, m_active->query, QDateTime::currentDateTimeUtc(), &page,
                       &error)) {
        finish({}, error);
        return;
    }
    finish(page, MarketCatalogError::ok());
}

void SteamMarketCatalogClient::retryOrFinish(const MarketCatalogError &error) {
    if (m_active.has_value() && m_active->retryCount < kMaximumNetworkRetries) {
        ++m_active->retryCount;
        scheduleDispatch(m_active->retryCount == 1 ? 30000 : 60000);
        return;
    }
    finish({}, error);
}

void SteamMarketCatalogClient::finish(const MarketCatalogPage &page,
                                      const MarketCatalogError &error) {
    if (!m_active.has_value()) return;
    MarketCatalogClientCallback callback = std::move(m_active->callback);
    m_active.reset();
    if (callback) callback(page, error);
}
