#pragma once

#include <QDateTime>
#include <QString>

// 模拟交易记录（对应 api-contract.yaml TradeRecord）。
struct TradeRecord {
    enum class Side { kBuy, kSell };

    int id = 0;
    QString marketHashName;
    int appid = 730;
    Side side = Side::kBuy;
    int quantity = 1;
    double price = 0.0;
    double fee = 0.0;
    double total = 0.0;
    QDateTime tradedAt;
    QString note;
};
