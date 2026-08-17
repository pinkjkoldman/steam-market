#pragma once

#include <QObject>
#include <optional>

#include "core/models/MarketCatalog.h"

class MarketCatalogRepository;
class SteamMarketCatalogClient;

class FullMarketService : public QObject {
    Q_OBJECT

public:
    FullMarketService(MarketCatalogRepository *repository,
                      SteamMarketCatalogClient *client, QObject *parent = nullptr);

    void requestPage(const MarketCatalogQuery &query, bool forceRefresh = false);
    void cancel();
    std::optional<MarketCatalogPage> cachedPage(const MarketCatalogQuery &query,
                                                bool allowExpired = true) const;
    QVector<MarketScopeSnapshot> scopeSnapshots(const QStringList &scopeKeys = {}) const;

signals:
    void pageReady(const MarketCatalogPage &page);
    void pageFailed(const MarketCatalogError &error);
    void requestStateChanged(bool inFlight);

private:
    QDateTime expiresAtFor(const MarketCatalogPage &page) const;

    MarketCatalogRepository *m_repository = nullptr;
    SteamMarketCatalogClient *m_client = nullptr;
    quint64 m_generation = 0;
    bool m_inFlight = false;
};
