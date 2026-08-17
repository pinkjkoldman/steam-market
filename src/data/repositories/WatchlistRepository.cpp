#include "data/repositories/WatchlistRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

WatchlistRepository::WatchlistRepository(QSqlDatabase &db) : m_db(db) {}

QVector<WatchlistItem> WatchlistRepository::all() const {
    QVector<WatchlistItem> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT id, market_hash_name, appid, added_at, note, sort_order "
            "FROM watchlist ORDER BY sort_order, id"))) {
        qWarning() << "读取自选失败:" << q.lastError().text();
        return out;
    }
    while (q.next()) {
        WatchlistItem it;
        it.id = q.value(0).toInt();
        it.marketHashName = q.value(1).toString();
        it.appid = q.value(2).toInt();
        it.addedAt = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
        it.note = q.value(4).toString();
        it.sortOrder = q.value(5).toInt();
        out.append(it);
    }
    return out;
}

bool WatchlistRepository::add(const QString &marketHashName, int appid, const QString &note) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO watchlist (market_hash_name, appid, added_at, note, sort_order) "
        "VALUES (?, ?, datetime('now'), ?, (SELECT COALESCE(MAX(sort_order),0)+1 FROM watchlist)) "
        "ON CONFLICT(market_hash_name) DO NOTHING"));
    q.addBindValue(marketHashName);
    q.addBindValue(appid);
    q.addBindValue(note.isEmpty() ? QVariant() : QVariant(note));
    if (!q.exec()) {
        qWarning() << "加入自选失败:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool WatchlistRepository::remove(const QString &marketHashName) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM watchlist WHERE market_hash_name = ?"));
    q.addBindValue(marketHashName);
    if (!q.exec()) {
        qWarning() << "移除自选失败:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool WatchlistRepository::contains(const QString &marketHashName) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM watchlist WHERE market_hash_name = ?"));
    q.addBindValue(marketHashName);
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

bool WatchlistRepository::setNote(const QString &marketHashName, const QString &note) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE watchlist SET note = ? WHERE market_hash_name = ?"));
    q.addBindValue(note);
    q.addBindValue(marketHashName);
    return q.exec();
}
