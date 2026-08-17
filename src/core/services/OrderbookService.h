#pragma once

#include <QObject>

#include "core/models/Orderbook.h"
#include "network/SteamMarketClient.h"

class OrderbookRepository;

// 盘口服务：调 itemordershistogram，规范化排序，缓存最新一份，失败降级。
class OrderbookService : public QObject {
    Q_OBJECT

public:
    OrderbookService(SteamMarketClient *client, OrderbookRepository *repo, QObject *parent = nullptr);

    void loadOrderbook(const QString &marketHashName, int appid);
    Orderbook cached(const QString &marketHashName) const;

signals:
    void orderbookReady(const QString &marketHashName, const Orderbook &orderbook,
                        const QString &errorMessage);

private:
    SteamMarketClient *m_client = nullptr;
    OrderbookRepository *m_repo = nullptr;
};
