#include "data/repositories/MarketCatalogRepository.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <cmath>
#include <limits>

namespace {
QString sortName(CatalogSort sort) {
    switch (sort) {
    case CatalogSort::kPriceAsc:
        return QStringLiteral("price_asc");
    case CatalogSort::kPriceDesc:
        return QStringLiteral("price_desc");
    case CatalogSort::kNameAsc:
        return QStringLiteral("name_asc");
    case CatalogSort::kPopular:
        return QStringLiteral("popular");
    }
    return QStringLiteral("popular");
}

QString scopeKey(const MarketCatalogQuery &query) {
    return query.appid.has_value()
               ? QStringLiteral("appid:%1").arg(query.appid.value())
               : QStringLiteral("all");
}

QJsonObject queryObject(const MarketCatalogQuery &query) {
    QStringList filters = query.categoryFilters;
    filters.removeAll(QString());
    filters.removeDuplicates();
    filters.sort(Qt::CaseSensitive);
    QJsonArray filterArray;
    for (const QString &filter : filters) filterArray.append(filter);
    QJsonObject object;
    object.insert(QStringLiteral("query"), query.query.normalized(QString::NormalizationForm_C));
    object.insert(QStringLiteral("appid"),
                  query.appid.has_value() ? QJsonValue(query.appid.value()) : QJsonValue::Null);
    object.insert(QStringLiteral("filters"), filterArray);
    object.insert(QStringLiteral("sort"), sortName(query.sort));
    object.insert(QStringLiteral("direction"),
                  query.sort == CatalogSort::kPriceAsc || query.sort == CatalogSort::kNameAsc
                      ? QStringLiteral("asc")
                      : QStringLiteral("desc"));
    object.insert(QStringLiteral("currency"), query.currency.toUpper());
    object.insert(QStringLiteral("language"), query.language.toLower());
    object.insert(QStringLiteral("offset"), query.offset);
    object.insert(QStringLiteral("limit"), query.limit);
    return object;
}

QByteArray serializeItems(const MarketCatalogPage &page) {
    QJsonArray items;
    for (const MarketCatalogItem &item : page.items) {
        QJsonObject object;
        object.insert(QStringLiteral("appid"), item.appid);
        object.insert(QStringLiteral("marketHashName"), item.marketHashName);
        object.insert(QStringLiteral("name"), item.localizedName);
        object.insert(QStringLiteral("typeText"), item.typeText);
        object.insert(QStringLiteral("iconUrl"), item.iconUrl);
        object.insert(QStringLiteral("lowestSellMinor"), item.lowestSellMinor);
        object.insert(QStringLiteral("sellListings"), item.sellListings);
        items.append(object);
    }
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("items"), items);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool safeInteger(const QJsonValue &value, qint64 minimum, qint64 maximum, qint64 *result) {
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < static_cast<double>(minimum)
        || number > static_cast<double>(maximum) || std::floor(number) != number) {
        return false;
    }
    *result = static_cast<qint64>(number);
    return true;
}

bool parseItems(const QByteArray &payload, int pageSize, QVector<MarketCatalogItem> *items) {
    if (!items || payload.size() > 2 * 1024 * 1024) return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt(-1) != 1
        || !root.value(QStringLiteral("items")).isArray()) {
        return false;
    }
    const QJsonArray array = root.value(QStringLiteral("items")).toArray();
    if (array.size() > pageSize || array.size() > 100) return false;
    QVector<MarketCatalogItem> parsed;
    parsed.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject()) return false;
        const QJsonObject object = value.toObject();
        MarketCatalogItem item;
        qint64 appid = 0;
        if (!safeInteger(object.value(QStringLiteral("appid")), 1,
                         std::numeric_limits<int>::max(), &appid)
            || !object.value(QStringLiteral("marketHashName")).isString()
            || object.value(QStringLiteral("marketHashName")).toString().isEmpty()
            || object.value(QStringLiteral("marketHashName")).toString().size() > 512
            || !object.value(QStringLiteral("name")).isString()
            || object.value(QStringLiteral("name")).toString().isEmpty()
            || object.value(QStringLiteral("name")).toString().size() > 512
            || !object.value(QStringLiteral("typeText")).isString()
            || object.value(QStringLiteral("typeText")).toString().size() > 256
            || !object.value(QStringLiteral("iconUrl")).isString()
            || object.value(QStringLiteral("iconUrl")).toString().size() > 2048
            || !safeInteger(object.value(QStringLiteral("lowestSellMinor")), 0,
                            std::numeric_limits<qint64>::max(), &item.lowestSellMinor)
            || !safeInteger(object.value(QStringLiteral("sellListings")), 0,
                            std::numeric_limits<qint64>::max(), &item.sellListings)) {
            return false;
        }
        item.appid = static_cast<int>(appid);
        item.marketHashName = object.value(QStringLiteral("marketHashName")).toString();
        item.localizedName = object.value(QStringLiteral("name")).toString();
        item.typeText = object.value(QStringLiteral("typeText")).toString();
        item.iconUrl = object.value(QStringLiteral("iconUrl")).toString();
        parsed.append(item);
    }
    *items = parsed;
    return true;
}

void setError(QString *error, const QString &message) {
    if (error) *error = message;
}
}  // namespace

MarketCatalogRepository::MarketCatalogRepository(QSqlDatabase &database)
    : m_database(database) {}

QString MarketCatalogRepository::canonicalQueryJson(const MarketCatalogQuery &query) {
    return QString::fromUtf8(QJsonDocument(queryObject(query)).toJson(QJsonDocument::Compact));
}

QString MarketCatalogRepository::cacheKey(const MarketCatalogQuery &query) {
    return QString::fromLatin1(QCryptographicHash::hash(canonicalQueryJson(query).toUtf8(),
                                                        QCryptographicHash::Sha256)
                                   .toHex());
}

bool MarketCatalogRepository::savePage(const MarketCatalogPage &page,
                                       const QDateTime &expiresAt, QString *error) {
    const QByteArray resultJson = serializeItems(page);
    if (resultJson.size() > 2 * 1024 * 1024 || page.origin != DataOrigin::kSteamLive
        || !page.fetchedAt.isValid() || !expiresAt.isValid() || page.offset < 0
        || page.pageSize < 1 || page.pageSize > 100 || page.totalCount < 0
        || page.totalCount > 10000000 || page.items.size() > page.pageSize) {
        setError(error, QStringLiteral("invalid live catalog page"));
        return false;
    }
    if (!m_database.transaction()) {
        setError(error, m_database.lastError().text());
        return false;
    }
    const QString fetchedAt = page.fetchedAt.toUTC().toString(Qt::ISODateWithMs);
    for (const MarketCatalogItem &item : page.items) {
        QSqlQuery identity(m_database);
        identity.prepare(QStringLiteral("SELECT appid FROM items WHERE market_hash_name=?"));
        identity.addBindValue(item.marketHashName);
        if (!identity.exec()) {
            m_database.rollback();
            setError(error, identity.lastError().text());
            return false;
        }
        if (identity.next() && identity.value(0).toInt() != item.appid) {
            m_database.rollback();
            setError(error, QStringLiteral("cross-app market_hash_name collision"));
            return false;
        }
        QSqlQuery itemQuery(m_database);
        itemQuery.prepare(QStringLiteral(
            "INSERT INTO items(market_hash_name,appid,name,icon_url,item_type,first_seen_at,updated_at,"
            "localized_name,sell_listings,lowest_sell_minor,catalog_seen_at,catalog_currency,catalog_language) "
            "VALUES(?,?,?,?,?,?,?, ?,?,?,?,?,?) ON CONFLICT(market_hash_name) DO UPDATE SET "
            "name=excluded.name,icon_url=CASE WHEN excluded.icon_url='' THEN items.icon_url ELSE excluded.icon_url END,"
            "item_type=excluded.item_type,updated_at=excluded.updated_at,localized_name=excluded.localized_name,"
            "sell_listings=excluded.sell_listings,lowest_sell_minor=excluded.lowest_sell_minor,"
            "catalog_seen_at=excluded.catalog_seen_at,catalog_currency=excluded.catalog_currency,"
            "catalog_language=excluded.catalog_language"));
        itemQuery.addBindValue(item.marketHashName);
        itemQuery.addBindValue(item.appid);
        itemQuery.addBindValue(item.localizedName);
        itemQuery.addBindValue(item.iconUrl);
        itemQuery.addBindValue(item.typeText);
        itemQuery.addBindValue(fetchedAt);
        itemQuery.addBindValue(fetchedAt);
        itemQuery.addBindValue(item.localizedName);
        itemQuery.addBindValue(item.sellListings);
        itemQuery.addBindValue(item.lowestSellMinor);
        itemQuery.addBindValue(fetchedAt);
        itemQuery.addBindValue(page.query.currency);
        itemQuery.addBindValue(page.query.language);
        if (!itemQuery.exec()) {
            m_database.rollback();
            setError(error, itemQuery.lastError().text());
            return false;
        }
    }
    QSqlQuery cache(m_database);
    cache.prepare(QStringLiteral(
        "INSERT INTO market_catalog_pages(cache_key,query_json,appid,offset,page_size,total_count,"
        "result_json,currency,language,fetched_at,expires_at,last_accessed_at) VALUES(?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(cache_key) DO UPDATE SET query_json=excluded.query_json,appid=excluded.appid,"
        "offset=excluded.offset,page_size=excluded.page_size,total_count=excluded.total_count,"
        "result_json=excluded.result_json,currency=excluded.currency,language=excluded.language,"
        "fetched_at=excluded.fetched_at,expires_at=excluded.expires_at,last_accessed_at=excluded.last_accessed_at"));
    cache.addBindValue(cacheKey(page.query));
    cache.addBindValue(canonicalQueryJson(page.query));
    cache.addBindValue(page.query.appid.has_value() ? QVariant(page.query.appid.value()) : QVariant());
    cache.addBindValue(page.offset);
    cache.addBindValue(page.pageSize);
    cache.addBindValue(page.totalCount);
    cache.addBindValue(QString::fromUtf8(resultJson));
    cache.addBindValue(page.query.currency);
    cache.addBindValue(page.query.language);
    cache.addBindValue(fetchedAt);
    cache.addBindValue(expiresAt.toUTC().toString(Qt::ISODateWithMs));
    cache.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!cache.exec()) {
        m_database.rollback();
        setError(error, cache.lastError().text());
        return false;
    }
    const bool isScopeSnapshot = page.query.query.trimmed().isEmpty()
                                 && page.query.categoryFilters.isEmpty()
                                 && page.offset == 0;
    if (isScopeSnapshot) {
        const QString key = scopeKey(page.query);
        QDateTime hourStart = page.fetchedAt.toUTC();
        hourStart.setTime(QTime(hourStart.time().hour(), 0));
        QSqlQuery removeSameHour(m_database);
        removeSameHour.prepare(QStringLiteral(
            "DELETE FROM market_scope_snapshots WHERE scope_key=? AND fetched_at>=? AND fetched_at<?"));
        removeSameHour.addBindValue(key);
        removeSameHour.addBindValue(hourStart.toString(Qt::ISODateWithMs));
        removeSameHour.addBindValue(hourStart.addSecs(3600).toString(Qt::ISODateWithMs));
        if (!removeSameHour.exec()) {
            m_database.rollback();
            setError(error, removeSameHour.lastError().text());
            return false;
        }
        QSqlQuery snapshot(m_database);
        snapshot.prepare(QStringLiteral(
            "INSERT INTO market_scope_snapshots(scope_key,total_count,fetched_at,source) VALUES(?,?,?,"
            "'steam_market_search')"));
        snapshot.addBindValue(key);
        snapshot.addBindValue(page.totalCount);
        snapshot.addBindValue(fetchedAt);
        if (!snapshot.exec()) {
            m_database.rollback();
            setError(error, snapshot.lastError().text());
            return false;
        }
    }
    if (!m_database.commit()) {
        setError(error, m_database.lastError().text());
        return false;
    }
    return true;
}

std::optional<MarketCatalogPage> MarketCatalogRepository::page(
    const MarketCatalogQuery &query, bool allowExpired, QString *error) const {
    QSqlQuery select(m_database);
    select.prepare(QStringLiteral(
        "SELECT query_json,offset,page_size,total_count,result_json,fetched_at,expires_at "
        "FROM market_catalog_pages WHERE cache_key=?"));
    select.addBindValue(cacheKey(query));
    if (!select.exec()) {
        setError(error, select.lastError().text());
        return std::nullopt;
    }
    if (!select.next()) return std::nullopt;
    if (select.value(0).toString() != canonicalQueryJson(query)) {
        setError(error, QStringLiteral("catalog cache key mismatch"));
        return std::nullopt;
    }
    const QDateTime fetchedAt = QDateTime::fromString(select.value(5).toString(), Qt::ISODate);
    const QDateTime expiresAt = QDateTime::fromString(select.value(6).toString(), Qt::ISODate);
    const bool stale = !expiresAt.isValid() || expiresAt < QDateTime::currentDateTimeUtc();
    if (stale && !allowExpired) return std::nullopt;
    MarketCatalogPage result;
    result.query = query;
    result.offset = select.value(1).toInt();
    result.pageSize = select.value(2).toInt();
    result.totalCount = select.value(3).toLongLong();
    result.fetchedAt = fetchedAt;
    result.origin = DataOrigin::kSteamCached;
    result.stale = stale;
    if (!fetchedAt.isValid() || result.offset < 0 || result.pageSize < 1
        || result.pageSize > 100 || result.totalCount < 0 || result.totalCount > 10000000
        || !parseItems(select.value(4).toString().toUtf8(), result.pageSize, &result.items)) {
        setError(error, QStringLiteral("catalog cache schema invalid"));
        return std::nullopt;
    }
    QSqlQuery touch(m_database);
    touch.prepare(QStringLiteral(
        "UPDATE market_catalog_pages SET last_accessed_at=? WHERE cache_key=?"));
    touch.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    touch.addBindValue(cacheKey(query));
    touch.exec();
    return result;
}

QVector<MarketScopeSnapshot> MarketCatalogRepository::latestScopeSnapshots(
    const QStringList &scopeKeys) const {
    QVector<MarketScopeSnapshot> snapshots;
    QString sql = QStringLiteral(
        "SELECT s.scope_key,s.total_count,s.fetched_at FROM market_scope_snapshots s "
        "JOIN (SELECT scope_key,MAX(fetched_at) AS fetched_at FROM market_scope_snapshots");
    if (!scopeKeys.isEmpty()) {
        QStringList placeholders;
        placeholders.fill(QStringLiteral("?"), scopeKeys.size());
        sql += QStringLiteral(" WHERE scope_key IN (%1)").arg(placeholders.join(QLatin1Char(',')));
    }
    sql += QStringLiteral(
        " GROUP BY scope_key) latest ON latest.scope_key=s.scope_key AND latest.fetched_at=s.fetched_at "
        "ORDER BY s.scope_key");
    QSqlQuery query(m_database);
    query.prepare(sql);
    for (const QString &key : scopeKeys) query.addBindValue(key);
    if (!query.exec()) return snapshots;
    while (query.next()) {
        MarketScopeSnapshot snapshot;
        snapshot.scopeKey = query.value(0).toString();
        snapshot.totalCount = query.value(1).toLongLong();
        snapshot.fetchedAt = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        snapshot.origin = DataOrigin::kSteamCached;
        snapshots.append(snapshot);
    }
    return snapshots;
}

bool MarketCatalogRepository::prune(const QDateTime &now, QString *error) {
    if (!m_database.transaction()) {
        setError(error, m_database.lastError().text());
        return false;
    }
    QSqlQuery oldPages(m_database);
    oldPages.prepare(QStringLiteral(
        "DELETE FROM market_catalog_pages WHERE fetched_at < ?"));
    oldPages.addBindValue(now.toUTC().addDays(-30).toString(Qt::ISODateWithMs));
    if (!oldPages.exec()) {
        m_database.rollback();
        setError(error, oldPages.lastError().text());
        return false;
    }
    QSqlQuery capacity(m_database);
    if (!capacity.exec(QStringLiteral(
            "DELETE FROM market_catalog_pages WHERE cache_key IN (SELECT cache_key FROM "
            "market_catalog_pages ORDER BY last_accessed_at DESC LIMIT -1 OFFSET 5000)"))) {
        m_database.rollback();
        setError(error, capacity.lastError().text());
        return false;
    }
    QSqlQuery snapshots(m_database);
    snapshots.prepare(QStringLiteral(
        "DELETE FROM market_scope_snapshots WHERE fetched_at < ?"));
    snapshots.addBindValue(now.toUTC().addDays(-180).toString(Qt::ISODateWithMs));
    if (!snapshots.exec()) {
        m_database.rollback();
        setError(error, snapshots.lastError().text());
        return false;
    }
    if (!m_database.commit()) {
        setError(error, m_database.lastError().text());
        return false;
    }
    return true;
}
