#pragma once

#include <QDate>

// 日 K 线（对应 api-contract.yaml KlineBar）。
struct KlineBar {
    QDate date;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    qint64 volume = 0;
    bool hasVolume = false;
    double amount = 0.0;
    double ma5 = 0.0;
    double ma10 = 0.0;
    double ma20 = 0.0;
};
