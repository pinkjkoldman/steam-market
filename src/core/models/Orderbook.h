#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

// 盘口条目（对应 api-contract.yaml OrderbookEntry）。
struct OrderbookEntry {
    double price = 0.0;
    int count = 0;
};

// 买卖盘口（对应 api-contract.yaml Orderbook）。
struct Orderbook {
    QString marketHashName;
    QVector<OrderbookEntry> buyOrders;
    QVector<OrderbookEntry> sellOrders;
    double highestBuy = -1.0;
    double lowestSell = -1.0;
    QDateTime fetchedAt;
    bool stale = false;
};
