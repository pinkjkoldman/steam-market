#include "core/services/FullMarketService.h"

#include "data/repositories/MarketCatalogRepository.h"
#include "network/SteamMarketCatalogClient.h"

FullMarketService::FullMarketService(MarketCatalogRepository *repository,
                                     SteamMarketCatalogClient *client, QObject *parent)
    : QObject(parent), m_repository(repository), m_client(client) {}

void FullMarketService::requestPage(const MarketCatalogQuery &query, bool forceRefresh) {
    const quint64 generation = ++m_generation;
    m_client->cancelCurrent();
    QString cacheError;
    std::optional<MarketCatalogPage> cached = m_repository->page(query, true, &cacheError);
    if (cached.has_value() && !cached->stale && !forceRefresh) {
        if (m_inFlight) {
            m_inFlight = false;
            emit requestStateChanged(false);
        }
        emit pageReady(cached.value());
        return;
    }
    if (cached.has_value()) emit pageReady(cached.value());
    m_inFlight = true;
    emit requestStateChanged(true);
    m_client->fetchPage(
        query, [this, generation, cached](const MarketCatalogPage &page,
                                         const MarketCatalogError &error) mutable {
            if (generation != m_generation) return;
            m_inFlight = false;
            emit requestStateChanged(false);
            if (!error.isOk()) {
                if (cached.has_value()) {
                    cached->origin = DataOrigin::kSteamCached;
                    cached->stale = true;
                    emit pageReady(cached.value());
                    return;
                }
                MarketCatalogError finalError = error;
                if (error.code != MarketCatalogErrorCode::kRateLimited
                    && error.code != MarketCatalogErrorCode::kSchemaChanged
                    && error.code != MarketCatalogErrorCode::kPageOutOfRange
                    && error.code != MarketCatalogErrorCode::kCurrencyUnsupported) {
                    finalError = MarketCatalogError::make(
                        MarketCatalogErrorCode::kOfflineNoCache,
                        QStringLiteral("market.offline_no_cache"), true, error.detail);
                }
                emit pageFailed(finalError);
                return;
            }
            QString databaseError;
            if (!m_repository->savePage(page, expiresAtFor(page), &databaseError)) {
                emit pageFailed(MarketCatalogError::make(
                    MarketCatalogErrorCode::kSchemaChanged,
                    QStringLiteral("market.cache_write_failed"), true, databaseError));
                return;
            }
            emit pageReady(page);
        });
}

void FullMarketService::cancel() {
    ++m_generation;
    m_client->cancelCurrent();
    if (m_inFlight) {
        m_inFlight = false;
        emit requestStateChanged(false);
    }
}

std::optional<MarketCatalogPage> FullMarketService::cachedPage(
    const MarketCatalogQuery &query, bool allowExpired) const {
    return m_repository->page(query, allowExpired);
}

QVector<MarketScopeSnapshot> FullMarketService::scopeSnapshots(
    const QStringList &scopeKeys) const {
    return m_repository->latestScopeSnapshots(scopeKeys);
}

QDateTime FullMarketService::expiresAtFor(const MarketCatalogPage &page) const {
    const bool popularScopePage = page.query.query.trimmed().isEmpty()
                                  && page.query.categoryFilters.isEmpty()
                                  && page.query.sort == CatalogSort::kPopular
                                  && page.query.offset == 0;
    return page.fetchedAt.addSecs(popularScopePage ? 10 * 60 : 30 * 60);
}
