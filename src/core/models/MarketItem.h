#pragma once

#include <QString>

// 市场物品条目（对应 api-contract.yaml MarketItem）。
struct MarketItem {
    QString marketHashName;
    int appid = 730;
    QString name;
    QString iconUrl;
    double price = 0.0;  // 最新价（分转元后）
    bool hasPrice = false;
    int volume = 0;  // 24h 销量
};
