#pragma once

#include <QString>

// 排行榜条目（对应 api-contract.yaml TopItem）。
struct TopItem {
    QString marketHashName;
    QString name;
    int appid = 730;
    double price = -1.0;
    double value = 0.0;  // 指标值：销量 或 涨跌幅%
    int rank = 0;
};
