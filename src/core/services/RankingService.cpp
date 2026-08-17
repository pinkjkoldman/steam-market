#include "core/services/RankingService.h"

#include <algorithm>

#include "data/repositories/ItemRepository.h"
#include "data/repositories/PriceRepository.h"
#include "data/repositories/WatchlistRepository.h"
#include "utils/CurrencyProvider.h"

RankingService::RankingService(WatchlistRepository *watchlist, PriceRepository *prices,
                               ItemRepository *items, QObject *parent)
    : QObject(parent), m_watchlist(watchlist), m_prices(prices), m_items(items) {}

QVector<TopItem> RankingService::top(By by, int limit) const {
    QVector<TopItem> out;
    // 标的池：自选 + 有快照数据的全部物品（去重）。
    QVector<WatchlistItem> pool = m_watchlist->all();
    QStringList names;
    for (const WatchlistItem &it : pool) {
        names << it.marketHashName;
    }
    names.removeDuplicates();

    for (const QString &name : names) {
        int appid = 730;
        for (const WatchlistItem &it : pool) {
            if (it.marketHashName == name) {
                appid = it.appid;
                break;
            }
        }
        const PriceOverview snap = m_prices->latestSnapshot(name, appid, CurrencyProvider::code());
        if (snap.priceHigh <= 0 && snap.priceLow <= 0) continue;
        const double price = snap.priceHigh > 0 ? snap.priceHigh : snap.priceLow;
        TopItem item;
        item.marketHashName = name;
        item.name = name;
        item.appid = appid;
        item.price = price;
        if (by == By::kVolume) {
            if (snap.volume < 0) continue;
            item.value = snap.volume;
        } else {
            const double past = m_prices->priceAtOrBefore(
                name, appid, CurrencyProvider::code(), snap.updatedAt.addDays(-1));
            if (past <= 0) continue;
            const double pct = (price - past) / past * 100.0;
            if (by == By::kGain && pct <= 0) continue;
            if (by == By::kLoss && pct >= 0) continue;
            item.value = by == By::kLoss ? -pct : pct;
        }
        out.append(item);
    }

    std::sort(out.begin(), out.end(), [by](const TopItem &a, const TopItem &b) {
        return by == By::kLoss ? a.value < b.value : a.value > b.value;
    });
    if (out.size() > limit) {
        out.resize(limit);
    }
    for (int i = 0; i < out.size(); ++i) {
        out[i].rank = i + 1;
    }
    return out;
}
