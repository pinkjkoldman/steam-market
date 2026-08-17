#pragma once

#include <QObject>
#include <QVector>

#include "core/models/MarketItem.h"
#include "core/models/PriceOverview.h"
#include "core/models/PricePoint.h"
#include "network/SteamMarketClient.h"

class ItemRepository;
class PriceRepository;

// 市场行情服务：搜索、详情（当前价 + 历史），结果写入缓存并通知 UI。
class MarketService : public QObject {
    Q_OBJECT

public:
    MarketService(SteamMarketClient *client, ItemRepository *items, PriceRepository *prices,
                  QObject *parent = nullptr);

    void search(const QString &query, int appid);
    void fetchDetail(const QString &marketHashName, int appid);
    void fetchOverview(const QString &marketHashName, int appid);

    PriceOverview cachedOverview(const QString &marketHashName, int appid) const;
    QVector<PricePoint> cachedHistory(const QString &marketHashName, int appid) const;
    QString iconUrl(const QString &marketHashName) const;

signals:
    void searchFinished(const QVector<MarketItem> &items, const QString &errorMessage);
    void detailReady(const QString &marketHashName, const PriceOverview &overview,
                     const QVector<PricePoint> &history, const QString &errorMessage);
    void overviewUpdated(const QString &marketHashName, const PriceOverview &overview,
                         const QString &errorMessage);

private:
    SteamMarketClient *m_client = nullptr;
    ItemRepository *m_items = nullptr;
    PriceRepository *m_prices = nullptr;
};
