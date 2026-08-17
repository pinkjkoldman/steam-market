#include "data/repositories/ItemRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

ItemRepository::ItemRepository(QSqlDatabase &db) : m_db(db) {}

bool ItemRepository::upsert(const MarketItem &item) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO items (market_hash_name, appid, name, icon_url, first_seen_at, updated_at) "
        "VALUES (?, ?, ?, ?, datetime('now'), datetime('now')) "
        "ON CONFLICT(market_hash_name) DO UPDATE SET "
        " appid=excluded.appid, name=excluded.name, "
        " icon_url=COALESCE(excluded.icon_url, items.icon_url), updated_at=datetime('now')"));
    q.addBindValue(item.marketHashName);
    q.addBindValue(item.appid);
    q.addBindValue(item.name);
    q.addBindValue(item.iconUrl.isEmpty() ? QVariant() : QVariant(item.iconUrl));
    if (!q.exec()) {
        qWarning() << "upsert item 失败:" << q.lastError().text();
        return false;
    }
    return true;
}

QString ItemRepository::iconUrlOf(const QString &marketHashName) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT icon_url FROM items WHERE market_hash_name = ?"));
    q.addBindValue(marketHashName);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}
