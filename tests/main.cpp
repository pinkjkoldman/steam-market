#include <QCoreApplication>
#include <QDir>
#include <QtTest>

#include "test_alerts.h"
#include "test_account_entry.h"
#include "test_full_market.h"
#include "test_csv.h"
#include "test_database.h"
#include "test_fee.h"
#include "test_kline.h"
#include "test_orderbook.h"
#include "test_portfolio.h"
#include "test_trades.h"
#include "test_inventory.h"

// 聚合运行全部测试类；任一失败返回非 0。
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    int result = 0;
    const QString logDir = qEnvironmentVariable("SMT_QTEST_LOG_DIR");
    if (!logDir.isEmpty()) {
        QDir().mkpath(logDir);
    }
    const auto run = [&](QObject *test, const char *className) {
        if (logDir.isEmpty()) {
            return QTest::qExec(test, argc, argv);
        }
        QByteArray outputSpec = QDir(logDir)
                                    .filePath(QString::fromLatin1(className) + QStringLiteral(".txt"))
                                    .toLocal8Bit();
        outputSpec.append(",txt");
        char outputOption[] = "-o";
        char *testArgv[] = {argv[0], outputOption, outputSpec.data()};
        return QTest::qExec(test, 3, testArgv);
    };
    TestDatabase t1;
    result |= run(&t1, "TestDatabase");
    TestAlerts t2;
    result |= run(&t2, "TestAlerts");
    TestPortfolio t3;
    result |= run(&t3, "TestPortfolio");
    TestCsv t4;
    result |= run(&t4, "TestCsv");
    TestKline t5;
    result |= run(&t5, "TestKline");
    TestFee t6;
    result |= run(&t6, "TestFee");
    TestTrades t7;
    result |= run(&t7, "TestTrades");
    TestOrderbook t8;
    result |= run(&t8, "TestOrderbook");
    TestInventory t9;
    result |= run(&t9, "TestInventory");
    TestAccountEntry t10;
    result |= run(&t10, "TestAccountEntry");
    TestFullMarket t11;
    result |= run(&t11, "TestFullMarket");
    return result;
}
