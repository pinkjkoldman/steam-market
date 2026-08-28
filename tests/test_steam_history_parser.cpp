#include "test_steam_history_parser.h"

#include <QTimeZone>
#include <QtTest>

#include "network/SteamHistoryParser.h"

void TestSteamHistoryParser::parsesRealShapeAndStringVolumes() {
    const QByteArray body = R"({
        "success": true,
        "price_prefix": "¥ ",
        "price_suffix": "",
        "prices": [
            ["Aug 01 2026 01: +0", 12.34, "1,234"],
            ["Aug 02 2026 01: +0", "13.50", 25]
        ]
    })";

    const SteamHistoryParseResult result = SteamHistoryParser::parse(body);
    QVERIFY2(result.error.isOk(), qPrintable(result.error.message));
    QCOMPARE(result.points.size(), 2);
    QCOMPARE(result.points.at(0).price, 12.34);
    QCOMPARE(result.points.at(0).volume, 1234);
    QVERIFY(result.points.at(0).hasVolume);
    QCOMPARE(result.points.at(0).recordedAt.timeSpec(), Qt::UTC);
    QCOMPARE(result.points.at(0).recordedAt,
             QDateTime(QDate(2026, 8, 1), QTime(1, 0), QTimeZone::UTC));
    QCOMPARE(result.points.at(1).price, 13.5);
    QCOMPARE(result.points.at(1).volume, 25);
}

void TestSteamHistoryParser::sortsAndDeduplicatesTimestamps() {
    const QByteArray body = R"({"success":true,"prices":[
        ["Aug 03 2026 01: +0", 3.0, "3"],
        ["Aug 01 2026 01: +0", 1.0, "1"],
        ["Aug 03 2026 01: +0", 3.5, "4"],
        ["Aug 02 2026 01: +0", 2.0, "2"]
    ]})";

    const SteamHistoryParseResult result = SteamHistoryParser::parse(body);
    QVERIFY(result.error.isOk());
    QCOMPARE(result.points.size(), 3);
    QCOMPARE(result.duplicateRows, 1);
    QVERIFY(result.points.at(0).recordedAt < result.points.at(1).recordedAt);
    QVERIFY(result.points.at(1).recordedAt < result.points.at(2).recordedAt);
    QCOMPARE(result.points.last().price, 3.5);
    QCOMPARE(result.points.last().volume, 4);

    const SteamHistoryParseResult offset = SteamHistoryParser::parse(
        R"({"success":true,"prices":[["Aug 03 2026 01: +8",3.0,"3"]]})");
    QVERIFY(offset.error.isOk());
    QCOMPARE(offset.points.first().recordedAt,
             QDateTime(QDate(2026, 8, 2), QTime(17, 0), QTimeZone::UTC));
}

void TestSteamHistoryParser::distinguishesEmptyFromInvalid() {
    const SteamHistoryParseResult empty =
        SteamHistoryParser::parse(R"({"success":true,"prices":[]})");
    QVERIFY(empty.error.isOk());
    QVERIFY(empty.explicitEmpty);
    QVERIFY(empty.points.isEmpty());

    const SteamHistoryParseResult rejected =
        SteamHistoryParser::parse(R"({"success":false,"prices":[]})");
    QCOMPARE(rejected.error.code, ErrorCode::kAuthenticationRequired);
    QVERIFY(!rejected.explicitEmpty);
}

void TestSteamHistoryParser::rejectsSchemaDrift() {
    const SteamHistoryParseResult missing =
        SteamHistoryParser::parse(R"({"success":true,"history":[]})");
    QCOMPARE(missing.error.code, ErrorCode::kSourceInvalid);

    const SteamHistoryParseResult mostlyInvalid = SteamHistoryParser::parse(R"({
        "success":true,
        "prices":[
            ["bad date", 1.0, "1"],
            ["also bad", -2.0, "x"],
            ["Aug 01 2026 01: +0", 3.0, "3"]
        ]
    })");
    QCOMPARE(mostlyInvalid.error.code, ErrorCode::kSourceInvalid);
    QVERIFY(mostlyInvalid.points.isEmpty());
}
