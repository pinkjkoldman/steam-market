#include "core/services/WatchlistService.h"

#include "data/repositories/ItemRepository.h"
#include "data/repositories/PriceRepository.h"
#include "data/repositories/WatchlistRepository.h"
#include "utils/CurrencyProvider.h"

WatchlistService::WatchlistService(WatchlistRepository *repo, PriceRepository *prices,
                                   ItemRepository *items, SteamMarketClient *client,
                                   QObject *parent)
    : QObject(parent), m_repo(repo), m_prices(prices), m_items(items), m_client(client) {}

QVector<WatchlistItem> WatchlistService::items() const {
    QVector<WatchlistItem> out = m_repo->all();
    for (WatchlistItem &it : out) {
        const PriceOverview snap =
            m_prices->latestSnapshot(it.marketHashName, it.appid, CurrencyProvider::code());
        it.hasPrice = snap.priceHigh > 0 || snap.priceLow > 0;
        it.latestPrice = it.hasPrice ? (snap.priceHigh > 0 ? snap.priceHigh : snap.priceLow) : -1.0;
        it.volume = snap.volume;
        if (snap.updatedAt.isValid()) {
            const double past = m_prices->priceAtOrBefore(
                it.marketHashName, it.appid, CurrencyProvider::code(),
                snap.updatedAt.addDays(-1));
            if (past > 0) {
                it.changePercent24h = (it.latestPrice - past) / past * 100.0;
            }
        }
    }
    return out;
}

bool WatchlistService::add(const QString &marketHashName, int appid, const QString &note) {
    const bool ok = m_repo->add(marketHashName, appid, note);
    if (ok) emit listChanged();
    return ok;
}

bool WatchlistService::remove(const QString &marketHashName) {
    const bool ok = m_repo->remove(marketHashName);
    if (ok) emit listChanged();
    return ok;
}

bool WatchlistService::isWatched(const QString &marketHashName) const {
    return m_repo->contains(marketHashName);
}

void WatchlistService::refreshAll() {
    const QVector<WatchlistItem> list = m_repo->all();
    m_total = list.size();
    if (m_total == 0) {
        emit refreshFinished(QString());
        emit listChanged();
        return;
    }
    refreshNext(0);
}

void WatchlistService::refreshNext(int index) {
    const QVector<WatchlistItem> list = m_repo->all();
    if (index >= list.size() || index >= m_total) {
        emit listChanged();
        emit refreshFinished(QString());
        return;
    }
    const QString hashName = list.at(index).marketHashName;
    const int appid = list.at(index).appid;
    m_client->fetchOverview(hashName, appid,
                            [this, index, appid](PriceOverview overview, const AppError &err) {
        if (err.isOk() && (overview.priceLow > 0 || overview.priceHigh > 0)) {
            m_prices->saveSnapshot(overview, appid);
        }
        refreshNext(index + 1);
    });
}
