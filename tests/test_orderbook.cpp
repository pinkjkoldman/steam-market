#include "test_orderbook.h"

#include <QtTest>

#include "data/repositories/ItemRepository.h"
#include "data/repositories/OrderbookRepository.h"
#include "test_helpers.h"

void TestOrderbook::roundtrip() {
    TestDb f;
    QVERIFY(f.opened);
    ItemRepository items(f.handle);
    OrderbookRepository repo(f.handle);
    MarketItem item;
    item.marketHashName = QStringLiteral("BOOK1");
    item.name = item.marketHashName;
    items.upsert(item);

    Orderbook book;
    book.marketHashName = item.marketHashName;
    for (int i = 0; i < 3; ++i) {
        OrderbookEntry buy;
        buy.price = 90.0 - i;
        buy.count = 10 + i;
        book.buyOrders.append(buy);
        OrderbookEntry sell;
        sell.price = 95.0 + i;
        sell.count = 5 + i;
        book.sellOrders.append(sell);
    }
    book.highestBuy = 90.0;
    book.lowestSell = 95.0;
    book.steamCurrencyId = 23;
    book.fetchedAt = QDateTime::currentDateTimeUtc();
    QVERIFY(repo.save(book));

    const Orderbook read = repo.latest(item.marketHashName);
    QCOMPARE(read.buyOrders.size(), 3);
    QCOMPARE(read.sellOrders.size(), 3);
    QCOMPARE(read.highestBuy, 90.0);
    QCOMPARE(read.lowestSell, 95.0);
    QCOMPARE(read.steamCurrencyId, 23);
    QCOMPARE(read.buyOrders.first().price, 90.0);
    QCOMPARE(read.sellOrders.last().count, 7);
    QVERIFY(read.stale);
}
