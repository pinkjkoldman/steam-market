#pragma once

#include <QDateTime>
#include <QString>

// 自选项（对应 api-contract.yaml WatchlistItem）。
struct WatchlistItem {
    int id = 0;
    QString marketHashName;
    int appid = 730;
    QDateTime addedAt;
    QString note;
    int sortOrder = 0;
    double latestPrice = -1.0;
    double changePercent24h = 0.0;
    int volume = -1;
    bool hasPrice = false;
    QString lastError;
};
