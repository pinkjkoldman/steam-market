#include "data/repositories/TradeRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

TradeRepository::TradeRepository(QSqlDatabase &db) : m_db(db) {}

QVector<TradeRecord> TradeRepository::all() const {
    QVector<TradeRecord> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT id, market_hash_name, appid, side, quantity, price, fee, total, traded_at, note "
            "FROM trades ORDER BY traded_at DESC, id DESC"))) {
        qWarning() << "读取模拟交易失败:" << q.lastError().text();
        return out;
    }
    while (q.next()) {
        TradeRecord r;
        r.id = q.value(0).toInt();
        r.marketHashName = q.value(1).toString();
        r.appid = q.value(2).toInt();
        r.side = q.value(3).toString() == QLatin1String("sell") ? TradeRecord::Side::kSell
                                                                : TradeRecord::Side::kBuy;
        r.quantity = q.value(4).toInt();
        r.price = q.value(5).toDouble();
        r.fee = q.value(6).toDouble();
        r.total = q.value(7).toDouble();
        r.tradedAt = QDateTime::fromString(q.value(8).toString(), Qt::ISODate);
        r.note = q.value(9).toString();
        out.append(r);
    }
    return out;
}

bool TradeRepository::add(const TradeRecord &record) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO trades (market_hash_name, appid, side, quantity, price, fee, total, traded_at, note) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(record.marketHashName);
    q.addBindValue(record.appid);
    q.addBindValue(record.side == TradeRecord::Side::kSell ? QStringLiteral("sell")
                                                           : QStringLiteral("buy"));
    q.addBindValue(record.quantity);
    q.addBindValue(record.price);
    q.addBindValue(record.fee);
    q.addBindValue(record.total);
    q.addBindValue(record.tradedAt.isValid() ? record.tradedAt.toString(Qt::ISODate)
                                             : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(record.note.isEmpty() ? QVariant() : QVariant(record.note));
    if (!q.exec()) {
        qWarning() << "记录模拟交易失败:" << q.lastError().text();
        return false;
    }
    return true;
}

bool TradeRepository::remove(int id) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM trades WHERE id = ?"));
    q.addBindValue(id);
    return q.exec() && q.numRowsAffected() > 0;
}

int TradeRepository::holdings(const QString &marketHashName) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(CASE WHEN side='buy' THEN quantity ELSE -quantity END), 0) "
        "FROM trades WHERE market_hash_name = ?"));
    q.addBindValue(marketHashName);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}
