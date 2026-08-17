#pragma once

#include <QSqlDatabase>
#include <optional>

#include "core/models/MarketCatalog.h"

class MarketCatalogRepository {
public:
    explicit MarketCatalogRepository(QSqlDatabase &database);

    static QString canonicalQueryJson(const MarketCatalogQuery &query);
    static QString cacheKey(const MarketCatalogQuery &query);

    bool savePage(const MarketCatalogPage &page, const QDateTime &expiresAt,
                  QString *error = nullptr);
    std::optional<MarketCatalogPage> page(const MarketCatalogQuery &query,
                                          bool allowExpired = true,
                                          QString *error = nullptr) const;
    QVector<MarketScopeSnapshot> latestScopeSnapshots(const QStringList &scopeKeys = {}) const;
    bool prune(const QDateTime &now = QDateTime::currentDateTimeUtc(),
               QString *error = nullptr);

private:
    QSqlDatabase m_database;
};
