#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>

#include "core/models/PricePoint.h"

enum class HistoryDataState {
    kLoading,
    kSteamLive,
    kCached,
    kAuthRequired,
    kRateLimited,
    kEmpty,
    kUnavailable,
    kInvalidResponse,
};

// 详情页消费的历史数据快照。状态、来源和错误与数据点一起传递，
// 避免缓存或失败响应被误标为在线数据。
struct HistorySnapshot {
    QString marketHashName;
    int appid = 0;
    QString currency;
    QVector<PricePoint> points;
    HistoryDataState state = HistoryDataState::kLoading;
    QDateTime fetchedAt;
    QString message;
    bool persisted = true;
};

Q_DECLARE_METATYPE(HistorySnapshot)
