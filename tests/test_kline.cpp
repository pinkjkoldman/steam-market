#include "test_kline.h"

#include <QTimeZone>
#include <QtTest>

#include "core/services/KlineService.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/PriceRepository.h"
#include "test_helpers.h"

namespace {
void seedHistory(TestDb &f, const QString &name) {
    ItemRepository items(f.handle);
    PriceRepository prices(f.handle);
    MarketItem item;
    item.marketHashName = name;
    item.name = name;
    items.upsert(item);
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QVector<PricePoint> hist;
    for (int d = 10; d >= 0; --d) {
        for (int k = 0; k < 3; ++k) {
            PricePoint p;
            p.recordedAt = now.addDays(-d).addSecs(k * 3600);
            p.price = 90.0 + d + k;
            p.volume = 10 + k;
            hist.append(p);
        }
    }
    prices.saveHistory(name, 730, QStringLiteral("CNY"), hist);
}
}  // namespace

void TestKline::dailyAggregation() {
    TestDb f;
    QVERIFY(f.opened);
    const QString name = QStringLiteral("KLINE1");
    seedHistory(f, name);
    PriceRepository prices(f.handle);
    KlineService service(&prices);
    const QVector<KlineBar> bars = service.dailyBars(name, 730, QStringLiteral("CNY"), 30);
    QCOMPARE(bars.size(), 11);
    // 最旧一天：open=第一个点，close=最后一个点，high/low 极值
    const KlineBar first = bars.first();
    QCOMPARE(first.open, 100.0);   // 90 + d(10) + k(0)
    QCOMPARE(first.close, 102.0);  // 90 + 10 + 2
    QCOMPARE(first.high, 102.0);
    QCOMPARE(first.low, 100.0);
    QCOMPARE(first.volume, 33);  // 10+11+12
    QVERIFY(first.amount > 0);
}

void TestKline::movingAverage() {
    TestDb f;
    QVERIFY(f.opened);
    const QString name = QStringLiteral("KLINE2");
    seedHistory(f, name);
    PriceRepository prices(f.handle);
    KlineService service(&prices);
    const QVector<KlineBar> bars = service.dailyBars(name, 730, QStringLiteral("CNY"), 30);
    QVERIFY(bars.size() >= 6);
    // 第 6 根（index 5）应已有 MA5；MA 为最近 5 天收盘均值
    double sum = 0.0;
    for (int i = 1; i <= 5; ++i) {
        sum += bars.at(i).close;
    }
    QVERIFY(qAbs(bars.at(5).ma5 - sum / 5.0) < 0.001);
    QCOMPARE(bars.at(0).ma5, 0.0);
}

void TestKline::sortsPointsAndPreservesVolumeMeaning() {
    KlineService service(nullptr);
    const QDateTime day1(QDate(2026, 8, 1), QTime(10, 0), QTimeZone::LocalTime);
    const QDateTime day2 = day1.addDays(1);

    PricePoint laterDay;
    laterDay.recordedAt = day2;
    laterDay.price = 20.0;

    PricePoint day1Close;
    day1Close.recordedAt = day1.addSecs(3600);
    day1Close.price = 12.0;
    day1Close.hasVolume = true;
    day1Close.volume = 0;

    PricePoint day1Open;
    day1Open.recordedAt = day1;
    day1Open.price = 10.0;

    const QVector<KlineBar> bars =
        service.barsFromPoints({laterDay, day1Close, day1Open}, 0);
    QCOMPARE(bars.size(), 2);
    QCOMPARE(bars.first().open, 10.0);
    QCOMPARE(bars.first().close, 12.0);
    QVERIFY(bars.first().hasVolume);
    QCOMPARE(bars.first().volume, 0);
    QVERIFY(!bars.last().hasVolume);
}
