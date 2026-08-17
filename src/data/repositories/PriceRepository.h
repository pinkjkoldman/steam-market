#pragma once

#include <QString>

#include "core/models/PriceOverview.h"
#include "core/models/PricePoint.h"

class QSqlDatabase;

// 行情快照与历史仓储：upsert、最新价、24h 前价格、历史读取。
class PriceRepository {
public:
    explicit PriceRepository(QSqlDatabase &db);

    bool saveSnapshot(const PriceOverview &overview, int appid);
    bool saveHistory(const QString &marketHashName, int appid, const QString &currency,
                     const QVector<PricePoint> &points);
    PriceOverview latestSnapshot(const QString &marketHashName, int appid,
                                 const QString &currency) const;
    // 距离目标时间最近（不晚于）的快照价格；找不到返回 -1。
    double priceAtOrBefore(const QString &marketHashName, int appid, const QString &currency,
                           const QDateTime &time) const;
    QVector<PricePoint> history(const QString &marketHashName, int appid,
                                const QString &currency) const;

private:
    QSqlDatabase &m_db;
};
