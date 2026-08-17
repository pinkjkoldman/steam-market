#pragma once

#include <QObject>
#include <QVector>

#include "core/models/WatchlistItem.h"
#include "network/SteamMarketClient.h"

class WatchlistRepository;
class PriceRepository;
class ItemRepository;

// 自选服务：列表、增删、批量刷新最新价。
class WatchlistService : public QObject {
    Q_OBJECT

public:
    WatchlistService(WatchlistRepository *repo, PriceRepository *prices, ItemRepository *items,
                     SteamMarketClient *client, QObject *parent = nullptr);

    QVector<WatchlistItem> items() const;
    bool add(const QString &marketHashName, int appid, const QString &note);
    bool remove(const QString &marketHashName);
    bool isWatched(const QString &marketHashName) const;
    void refreshAll();

signals:
    void listChanged();
    void refreshFinished(const QString &errorMessage);

private:
    void refreshNext(int index);
    WatchlistRepository *m_repo = nullptr;
    PriceRepository *m_prices = nullptr;
    ItemRepository *m_items = nullptr;
    SteamMarketClient *m_client = nullptr;
    int m_total = 0;
};
