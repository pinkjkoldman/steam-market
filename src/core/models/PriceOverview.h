#pragma once

#include <QDateTime>
#include <QString>

// 当前价/销量概览（对应 api-contract.yaml PriceOverview）。
struct PriceOverview {
    QString marketHashName;
    QString currency;
    double priceLow = -1.0;
    double priceHigh = -1.0;
    int volume = -1;
    QDateTime updatedAt;
    bool stale = false;  // true 表示来自缓存降级
};
