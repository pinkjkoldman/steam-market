#pragma once

#include <QObject>
#include <QVector>

#include "core/models/TopItem.h"

class WatchlistRepository;
class PriceRepository;
class ItemRepository;

// 排行榜服务：基于本地快照/历史计算成交量榜、涨幅榜、跌幅榜。
class RankingService : public QObject {
    Q_OBJECT

public:
    enum class By { kVolume, kGain, kLoss };

    RankingService(WatchlistRepository *watchlist, PriceRepository *prices, ItemRepository *items,
                   QObject *parent = nullptr);

    QVector<TopItem> top(By by, int limit = 20) const;

private:
    WatchlistRepository *m_watchlist = nullptr;
    PriceRepository *m_prices = nullptr;
    ItemRepository *m_items = nullptr;
};
