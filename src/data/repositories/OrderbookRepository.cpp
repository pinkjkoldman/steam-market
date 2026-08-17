#include "data/repositories/OrderbookRepository.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

namespace {
QString entriesToJson(const QVector<OrderbookEntry> &entries) {
    QJsonArray arr;
    for (const OrderbookEntry &e : entries) {
        QJsonObject o;
        o.insert(QStringLiteral("price"), e.price);
        o.insert(QStringLiteral("count"), e.count);
        arr.append(o);
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QVector<OrderbookEntry> entriesFromJson(const QString &json) {
    QVector<OrderbookEntry> out;
    const QJsonArray arr = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        OrderbookEntry e;
        e.price = o.value(QStringLiteral("price")).toDouble();
        e.count = o.value(QStringLiteral("count")).toInt();
        out.append(e);
    }
    return out;
}
}  // namespace

OrderbookRepository::OrderbookRepository(QSqlDatabase &db) : m_db(db) {}

bool OrderbookRepository::save(const Orderbook &orderbook) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO orderbook_snapshots "
        " (market_hash_name, buy_orders_json, sell_orders_json, highest_buy, lowest_sell, fetched_at) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(market_hash_name) DO UPDATE SET "
        " buy_orders_json=excluded.buy_orders_json, sell_orders_json=excluded.sell_orders_json, "
        " highest_buy=excluded.highest_buy, lowest_sell=excluded.lowest_sell, fetched_at=excluded.fetched_at"));
    q.addBindValue(orderbook.marketHashName);
    q.addBindValue(entriesToJson(orderbook.buyOrders));
    q.addBindValue(entriesToJson(orderbook.sellOrders));
    q.addBindValue(orderbook.highestBuy > 0 ? QVariant(orderbook.highestBuy) : QVariant());
    q.addBindValue(orderbook.lowestSell > 0 ? QVariant(orderbook.lowestSell) : QVariant());
    q.addBindValue(orderbook.fetchedAt.isValid() ? orderbook.fetchedAt.toString(Qt::ISODate)
                                                 : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "保存盘口失败:" << q.lastError().text();
        return false;
    }
    return true;
}

Orderbook OrderbookRepository::latest(const QString &marketHashName) const {
    Orderbook out;
    out.marketHashName = marketHashName;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT buy_orders_json, sell_orders_json, highest_buy, lowest_sell, fetched_at "
        "FROM orderbook_snapshots WHERE market_hash_name = ?"));
    q.addBindValue(marketHashName);
    if (q.exec() && q.next()) {
        out.buyOrders = entriesFromJson(q.value(0).toString());
        out.sellOrders = entriesFromJson(q.value(1).toString());
        out.highestBuy = q.value(2).isNull() ? -1.0 : q.value(2).toDouble();
        out.lowestSell = q.value(3).isNull() ? -1.0 : q.value(3).toDouble();
        out.fetchedAt = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        out.stale = true;
    }
    return out;
}
