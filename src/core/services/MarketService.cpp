#include "core/services/MarketService.h"

#include "data/repositories/ItemRepository.h"
#include "data/repositories/PriceRepository.h"
#include "utils/CurrencyProvider.h"

MarketService::MarketService(SteamMarketClient *client, ItemRepository *items,
                             PriceRepository *prices, QObject *parent)
    : QObject(parent), m_client(client), m_items(items), m_prices(prices) {}

void MarketService::search(const QString &query, int appid) {
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) {
        emit searchFinished({}, QStringLiteral("请输入搜索关键词"));
        return;
    }
    m_client->search(trimmed.left(128), appid, [this](QVector<MarketItem> items, const AppError &err) {
        if (err.isOk()) {
            for (const MarketItem &item : items) {
                m_items->upsert(item);
            }
        }
        emit searchFinished(items, err.isOk() ? QString() : err.message);
    });
}

void MarketService::fetchDetail(const QString &marketHashName, int appid) {
    // 先给缓存，再异步刷新最新数据。
    const PriceOverview cached = cachedOverview(marketHashName, appid);
    const QVector<PricePoint> hist = cachedHistory(marketHashName, appid);
    if (cached.updatedAt.isValid() || !hist.isEmpty()) {
        emit detailReady(marketHashName, cached, hist, QString());
    }
    fetchOverview(marketHashName, appid);
    m_client->fetchHistory(marketHashName, appid,
                           [this, marketHashName, appid](QVector<PricePoint> points,
                                                         const AppError &err) {
        if (err.isOk()) {
            m_prices->saveHistory(marketHashName, appid, CurrencyProvider::code(), points);
        }
        emit detailReady(marketHashName, cachedOverview(marketHashName, appid),
                         err.isOk() ? m_prices->history(marketHashName, appid, CurrencyProvider::code())
                                    : cachedHistory(marketHashName, appid),
                         err.message);
    });
}

void MarketService::fetchOverview(const QString &marketHashName, int appid) {
    m_client->fetchOverview(marketHashName, appid,
                            [this, marketHashName, appid](PriceOverview overview,
                                                          const AppError &err) {
        if (err.isOk() && (overview.priceLow > 0 || overview.priceHigh > 0)) {
            m_prices->saveSnapshot(overview, appid);
        }
        emit overviewUpdated(marketHashName,
                             err.isOk() ? overview : cachedOverview(marketHashName, appid),
                             err.message);
    });
}

PriceOverview MarketService::cachedOverview(const QString &marketHashName, int appid) const {
    return m_prices->latestSnapshot(marketHashName, appid, CurrencyProvider::code());
}

QVector<PricePoint> MarketService::cachedHistory(const QString &marketHashName, int appid) const {
    return m_prices->history(marketHashName, appid, CurrencyProvider::code());
}

QString MarketService::iconUrl(const QString &marketHashName) const {
    return m_items->iconUrlOf(marketHashName);
}
