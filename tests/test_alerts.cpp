#include "test_alerts.h"

#include <QtTest>

#include "core/services/AlertService.h"
#include "data/repositories/AlertRepository.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/PriceRepository.h"
#include "test_helpers.h"

namespace {
void seedItemAndSnapshots(TestDb &f, const QString &name, double todayPrice, double yesterdayPrice) {
    ItemRepository items(f.handle);
    PriceRepository prices(f.handle);
    MarketItem item;
    item.marketHashName = name;
    item.name = name;
    items.upsert(item);
    const QDateTime now = QDateTime::currentDateTimeUtc();
    PriceOverview today;
    today.marketHashName = name;
    today.currency = QStringLiteral("CNY");
    today.priceHigh = todayPrice;
    today.priceLow = todayPrice - 1;
    today.updatedAt = now;
    prices.saveSnapshot(today, 730);
    PriceOverview yesterday = today;
    yesterday.priceHigh = yesterdayPrice;
    yesterday.priceLow = yesterdayPrice - 1;
    yesterday.updatedAt = now.addDays(-1);
    prices.saveSnapshot(yesterday, 730);
}
}  // namespace

void TestAlerts::triggersBelowAndAbove() {
    TestDb f;
    QVERIFY(f.opened);
    seedItemAndSnapshots(f, QStringLiteral("TEST1"), 100.0, 90.0);
    AlertRepository alerts(f.handle);
    PriceRepository prices(f.handle);
    AlertService service(&alerts, &prices);

    int triggered = 0;
    QString lastBody;
    connect(&service, &AlertService::alertTriggered, this,
            [&](const QString &, const QString &body) { ++triggered; lastBody = body; });

    Alert below;
    below.marketHashName = QStringLiteral("TEST1");
    below.conditionType = Alert::Condition::kBelow;
    below.thresholdValue = 150.0;  // 100 <= 150 → 命中
    alerts.add(below);
    Alert above;
    above.marketHashName = QStringLiteral("TEST1");
    above.conditionType = Alert::Condition::kAbove;
    above.thresholdValue = 50.0;  // 100 >= 50 → 命中
    alerts.add(above);
    Alert notMet;
    notMet.marketHashName = QStringLiteral("TEST1");
    notMet.conditionType = Alert::Condition::kAbove;
    notMet.thresholdValue = 500.0;  // 不命中
    alerts.add(notMet);

    service.checkAll();
    QCOMPARE(triggered, 2);
    QVERIFY(lastBody.contains(QStringLiteral("TEST1")));
}

void TestAlerts::cooldownPreventsRepeat() {
    TestDb f;
    QVERIFY(f.opened);
    seedItemAndSnapshots(f, QStringLiteral("TEST2"), 100.0, 90.0);
    AlertRepository alerts(f.handle);
    PriceRepository prices(f.handle);
    AlertService service(&alerts, &prices);

    int triggered = 0;
    connect(&service, &AlertService::alertTriggered, this,
            [&](const QString &, const QString &) { ++triggered; });
    Alert below;
    below.marketHashName = QStringLiteral("TEST2");
    below.conditionType = Alert::Condition::kBelow;
    below.thresholdValue = 150.0;
    alerts.add(below);

    service.checkAll();
    QCOMPARE(triggered, 1);
    service.checkAll();  // 冷却期内不应重复
    QCOMPARE(triggered, 1);
}

void TestAlerts::percentTrigger() {
    TestDb f;
    QVERIFY(f.opened);
    seedItemAndSnapshots(f, QStringLiteral("TEST3"), 120.0, 100.0);  // +20%
    AlertRepository alerts(f.handle);
    PriceRepository prices(f.handle);
    AlertService service(&alerts, &prices);

    int triggered = 0;
    connect(&service, &AlertService::alertTriggered, this,
            [&](const QString &, const QString &) { ++triggered; });
    Alert percent;
    percent.marketHashName = QStringLiteral("TEST3");
    percent.conditionType = Alert::Condition::kPercent24h;
    percent.percentValue = 10.0;  // 20% >= 10% → 命中
    alerts.add(percent);
    service.checkAll();
    QCOMPARE(triggered, 1);
}
