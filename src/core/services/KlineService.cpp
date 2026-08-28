#include "core/services/KlineService.h"

#include <QDate>
#include <QDateTime>

#include <algorithm>
#include <cmath>

#include "data/repositories/PriceRepository.h"

KlineService::KlineService(PriceRepository *prices, QObject *parent)
    : QObject(parent), m_prices(prices) {}

QVector<KlineBar> KlineService::dailyBars(const QString &marketHashName, int appid,
                                          const QString &currency,
                                          int maxDays) const {
    return barsFromPoints(m_prices->history(marketHashName, appid, currency), maxDays);
}

QVector<KlineBar> KlineService::barsFromPoints(const QVector<PricePoint> &points,
                                               int maxDays) const {
    QVector<KlineBar> bars;
    QVector<PricePoint> ordered;
    ordered.reserve(points.size());
    for (const PricePoint &point : points) {
        if (point.recordedAt.isValid() && std::isfinite(point.price) && point.price > 0.0) {
            ordered.append(point);
        }
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const PricePoint &left, const PricePoint &right) {
                  return left.recordedAt < right.recordedAt;
              });

    QDate cutoff;
    if (maxDays > 0) {
        cutoff = QDate::currentDate().addDays(-maxDays);
    }
    for (const PricePoint &p : ordered) {
        const QDate day = p.recordedAt.toLocalTime().date();
        if (cutoff.isValid() && day < cutoff) continue;
        if (bars.isEmpty() || bars.last().date != day) {
            KlineBar bar;
            bar.date = day;
            bar.open = bar.high = bar.low = bar.close = p.price;
            bar.hasVolume = p.hasVolume;
            if (p.hasVolume) {
                bar.volume = p.volume;
                bar.amount = p.price * p.volume;
            }
            bars.append(bar);
        } else {
            KlineBar &bar = bars.last();
            bar.high = qMax(bar.high, p.price);
            bar.low = qMin(bar.low, p.price);
            bar.close = p.price;
            if (p.hasVolume) {
                bar.hasVolume = true;
                bar.volume += p.volume;
                bar.amount += p.price * p.volume;
            }
        }
    }
    applyMovingAverages(bars);
    return bars;
}

QVector<PricePoint> KlineService::minutePoints(const QString &marketHashName, int appid,
                                               const QString &currency) const {
    QVector<PricePoint> out;
    const QDateTime cutoff = QDateTime::currentDateTimeUtc().addDays(-1);
    const QVector<PricePoint> points = m_prices->history(marketHashName, appid, currency);
    for (const PricePoint &p : points) {
        if (p.recordedAt >= cutoff) {
            out.append(p);
        }
    }
    return out;
}

void KlineService::applyMovingAverages(QVector<KlineBar> &bars) {
    const int windows[3] = {5, 10, 20};
    for (int i = 0; i < bars.size(); ++i) {
        for (int w : windows) {
            if (i + 1 < w) continue;
            double sum = 0.0;
            for (int j = i - w + 1; j <= i; ++j) {
                sum += bars.at(j).close;
            }
            const double ma = sum / w;
            if (w == 5) bars[i].ma5 = ma;
            if (w == 10) bars[i].ma10 = ma;
            if (w == 20) bars[i].ma20 = ma;
        }
    }
}
