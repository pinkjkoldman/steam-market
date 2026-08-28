#pragma once

#include <QObject>
#include <QSet>
#include <QVector>

#include "core/models/HistorySnapshot.h"
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

    void search(const QString &query, int appid, int start = 0);
    void fetchDetail(const QString &marketHashName, int appid);
    void fetchOverview(const QString &marketHashName, int appid);
    void refreshHistory(const QString &marketHashName, int appid);
    void setHistoryAuthenticated(bool authenticated) { m_historyAuthenticated = authenticated; }
    bool isHistoryAuthenticated() const { return m_historyAuthenticated; }

    PriceOverview cachedOverview(const QString &marketHashName, int appid) const;
    QVector<PricePoint> cachedHistory(const QString &marketHashName, int appid) const;
    QString iconUrl(const QString &marketHashName) const;

signals:
    void searchFinished(const QVector<MarketItem> &items, int totalCount,
                        const QString &errorMessage);
    void overviewUpdated(const QString &marketHashName, const PriceOverview &overview,
                         const QString &errorMessage);
    void historyUpdated(const HistorySnapshot &snapshot);
    void historyAuthenticationExpired();

private:
    void fetchHistory(const QString &marketHashName, int appid);
    SteamMarketClient *m_client = nullptr;
    ItemRepository *m_items = nullptr;
    PriceRepository *m_prices = nullptr;
    QSet<QString> m_historyInFlight;
    bool m_historyAuthenticated = false;
};
