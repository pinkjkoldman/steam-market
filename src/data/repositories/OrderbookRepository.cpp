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
QString entriesToJson(const QVector<OrderbookEntry> &entries, int steamCurrencyId) {
    QJsonArray arr;
    for (const OrderbookEntry &e : entries) {
        QJsonObject o;
        o.insert(QStringLiteral("price"), e.price);
        o.insert(QStringLiteral("count"), e.count);
        arr.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("currency_id"), steamCurrencyId);
    root.insert(QStringLiteral("entries"), arr);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QVector<OrderbookEntry> entriesFromJson(const QString &json, int *steamCurrencyId = nullptr) {
    QVector<OrderbookEntry> out;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    QJsonArray arr;
    if (document.isObject()) {
        const QJsonObject root = document.object();
        arr = root.value(QStringLiteral("entries")).toArray();
        if (steamCurrencyId) {
            *steamCurrencyId = root.value(QStringLiteral("currency_id")).toInt(0);
        }
    } else {
        // 兼容升级前只存储档位数组的缓存。
        arr = document.array();
    }
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
    q.addBindValue(entriesToJson(orderbook.buyOrders, orderbook.steamCurrencyId));
    q.addBindValue(entriesToJson(orderbook.sellOrders, orderbook.steamCurrencyId));
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
        out.buyOrders = entriesFromJson(q.value(0).toString(), &out.steamCurrencyId);
        int sellCurrencyId = 0;
        out.sellOrders = entriesFromJson(q.value(1).toString(), &sellCurrencyId);
        if (out.steamCurrencyId == 0) out.steamCurrencyId = sellCurrencyId;
        out.highestBuy = q.value(2).isNull() ? -1.0 : q.value(2).toDouble();
        out.lowestSell = q.value(3).isNull() ? -1.0 : q.value(3).toDouble();
        out.fetchedAt = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        out.stale = true;
    }
    return out;
}
