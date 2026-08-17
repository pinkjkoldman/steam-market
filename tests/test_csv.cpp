#include "test_csv.h"

#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "price_sources/CsvPriceSource.h"
#include "test_helpers.h"

void TestCsv::importRows() {
    TestDb f;
    QVERIFY(f.opened);
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("prices.csv"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("market_hash_name,platform,price,currency,url\n"
               "AK-47 | Redline,buff,88.5,CNY,https://buff.example/a\n"
               "AWP | Asiimov,c5,910.0,CNY,https://c5.example/b\n");
    file.close();

    AppError err;
    CsvPriceSource source(f.handle);
    QCOMPARE(source.import(path, &err), 2);
    QVERIFY(err.isOk());

    QSqlQuery q(f.handle);
    QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM platform_prices")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 2);
}

void TestCsv::invalidFile() {
    TestDb f;
    QVERIFY(f.opened);
    AppError err;
    CsvPriceSource source(f.handle);
    QCOMPARE(source.import(QStringLiteral("Z:/definitely/missing.csv"), &err), -1);
    QVERIFY(!err.isOk());
}
