#include "core/services/OrderbookService.h"

#include <algorithm>

#include "data/repositories/OrderbookRepository.h"

OrderbookService::OrderbookService(SteamMarketClient *client, OrderbookRepository *repo,
                                   QObject *parent)
    : QObject(parent), m_client(client), m_repo(repo) {}

void OrderbookService::loadOrderbook(const QString &marketHashName, int appid) {
    // 先给缓存，再异步刷新。
    const Orderbook cachedBook = cached(marketHashName);
    if (!cachedBook.buyOrders.isEmpty() || !cachedBook.sellOrders.isEmpty()) {
        emit orderbookReady(marketHashName, cachedBook, QString());
    }
    m_client->fetchOrderbook(marketHashName, appid,
                             [this, marketHashName](Orderbook book, const AppError &err) {
        if (err.isOk()) {
            // 买盘按价格降序（最高买价在前），卖盘按价格升序（最低卖价在前）。
            std::sort(book.buyOrders.begin(), book.buyOrders.end(),
                      [](const OrderbookEntry &a, const OrderbookEntry &b) {
                return a.price > b.price;
            });
            std::sort(book.sellOrders.begin(), book.sellOrders.end(),
                      [](const OrderbookEntry &a, const OrderbookEntry &b) {
                return a.price < b.price;
            });
            if (book.buyOrders.isEmpty() && book.sellOrders.isEmpty()) {
                emit orderbookReady(marketHashName, cached(marketHashName),
                                    QStringLiteral("该物品暂无盘口数据"));
                return;
            }
            m_repo->save(book);
            emit orderbookReady(marketHashName, book, QString());
        } else {
            emit orderbookReady(marketHashName, cached(marketHashName),
                                QStringLiteral("盘口暂不可用：%1").arg(err.message));
        }
    });
}

Orderbook OrderbookService::cached(const QString &marketHashName) const {
    return m_repo->latest(marketHashName);
}
