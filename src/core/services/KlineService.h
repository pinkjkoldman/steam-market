#pragma once

#include <QObject>
#include <QVector>

#include "core/models/KlineBar.h"
#include "core/models/PricePoint.h"

class PriceRepository;

// 日 K 线聚合服务：从 price_history 按日聚合 OHLC + 成交量 + 均线（内存计算，不落库）。
class KlineService : public QObject {
    Q_OBJECT

public:
    explicit KlineService(PriceRepository *prices, QObject *parent = nullptr);

    QVector<KlineBar> dailyBars(const QString &marketHashName, int appid,
                                const QString &currency,
                                int maxDays = 0) const;
    // 当日分时点（最近 24h 快照/历史点）。
    QVector<PricePoint> minutePoints(const QString &marketHashName, int appid,
                                     const QString &currency) const;

private:
    static void applyMovingAverages(QVector<KlineBar> &bars);
    PriceRepository *m_prices = nullptr;
};
