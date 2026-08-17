#include "test_fee.h"

#include <QtTest>

#include "core/services/SettingsService.h"
#include "core/services/TradingRulesService.h"
#include "data/repositories/SettingsRepository.h"
#include "test_helpers.h"

void TestFee::sellDirection() {
    TestDb f;
    QVERIFY(f.opened);
    SettingsRepository settingsRepo(f.handle);
    SettingsService settings(&settingsRepo);
    TradingRulesService rules(&settings);

    const FeeEstimate fee = rules.estimateFee(QStringLiteral("sell"), 100.0);
    QCOMPARE(fee.sellerReceives, 100.0);
    QCOMPARE(fee.steamFee, 5.0);
    QCOMPARE(fee.gameFee, 10.0);
    QCOMPARE(fee.totalFee, 15.0);
    QCOMPARE(fee.buyerPays, 115.0);
}

void TestFee::buyDirection() {
    TestDb f;
    QVERIFY(f.opened);
    SettingsRepository settingsRepo(f.handle);
    SettingsService settings(&settingsRepo);
    TradingRulesService rules(&settings);

    const FeeEstimate fee = rules.estimateFee(QStringLiteral("buy"), 115.0);
    QVERIFY(qAbs(fee.sellerReceives - 100.0) < 0.01);
    QVERIFY(qAbs(fee.totalFee - 15.0) < 0.01);
}
