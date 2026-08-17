#pragma once

#include <QDateTime>

// 历史走势点（对应 api-contract.yaml PricePoint）。
struct PricePoint {
    QDateTime recordedAt;
    double price = 0.0;
    int volume = 0;
};
