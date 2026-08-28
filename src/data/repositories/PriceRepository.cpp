#include "data/repositories/PriceRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

PriceRepository::PriceRepository(QSqlDatabase &db) : m_db(db) {}

bool PriceRepository::saveSnapshot(const PriceOverview &overview, int appid) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO price_snapshots (market_hash_name, appid, currency, price_low, price_high, volume, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(market_hash_name, appid, currency, created_at) DO NOTHING"));
    q.addBindValue(overview.marketHashName);
    q.addBindValue(appid);
    q.addBindValue(overview.currency);
    q.addBindValue(overview.priceLow < 0 ? QVariant() : QVariant(overview.priceLow));
    q.addBindValue(overview.priceHigh < 0 ? QVariant() : QVariant(overview.priceHigh));
    q.addBindValue(overview.volume < 0 ? QVariant() : QVariant(overview.volume));
    q.addBindValue(overview.updatedAt.isValid() ? overview.updatedAt.toString(Qt::ISODate)
                                                : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "saveSnapshot 失败:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PriceRepository::saveHistory(const QString &marketHashName, int appid,
                                  const QString &currency, const QVector<PricePoint> &points) {
    m_db.transaction();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO price_history (market_hash_name, appid, currency, price, volume, recorded_at) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(market_hash_name, appid, currency, recorded_at) DO UPDATE SET "
        "price = excluded.price, volume = excluded.volume"));
    qint64 saved = 0;
    for (const PricePoint &p : points) {
        q.addBindValue(marketHashName);
        q.addBindValue(appid);
        q.addBindValue(currency);
        q.addBindValue(p.price);
        q.addBindValue((p.hasVolume || p.volume > 0) ? QVariant(p.volume) : QVariant());
        q.addBindValue(p.recordedAt.toString(Qt::ISODate));
        if (!q.exec()) {
            qWarning() << "saveHistory 失败:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
        ++saved;
    }
    const bool ok = m_db.commit();
    qInfo() << "saveHistory 保存" << saved << "条（" << marketHashName << "）";
    return ok;
}

PriceOverview PriceRepository::latestSnapshot(const QString &marketHashName, int appid,
                                              const QString &currency) const {
    PriceOverview out;
    out.marketHashName = marketHashName;
    out.currency = currency;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT price_low, price_high, volume, created_at FROM price_snapshots "
        "WHERE market_hash_name = ? AND appid = ? AND currency = ? ORDER BY created_at DESC LIMIT 1"));
    q.addBindValue(marketHashName);
    q.addBindValue(appid);
    q.addBindValue(currency);
    if (q.exec() && q.next()) {
        out.priceLow = q.value(0).isNull() ? -1.0 : q.value(0).toDouble();
        out.priceHigh = q.value(1).isNull() ? -1.0 : q.value(1).toDouble();
        out.volume = q.value(2).isNull() ? -1 : q.value(2).toInt();
        out.updatedAt = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
        out.stale = true;
    }
    return out;
}

double PriceRepository::priceAtOrBefore(const QString &marketHashName, int appid,
                                        const QString &currency, const QDateTime &time) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT price_low FROM price_snapshots "
        "WHERE market_hash_name = ? AND appid = ? AND currency = ? AND created_at <= ? "
        "ORDER BY created_at DESC LIMIT 1"));
    q.addBindValue(marketHashName);
    q.addBindValue(appid);
    q.addBindValue(currency);
    q.addBindValue(time.toString(Qt::ISODate));
    if (q.exec() && q.next() && !q.value(0).isNull()) {
        return q.value(0).toDouble();
    }
    return -1.0;
}

QVector<PricePoint> PriceRepository::history(const QString &marketHashName, int appid,
                                             const QString &currency) const {
    QVector<PricePoint> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT recorded_at, price, volume FROM price_history "
        "WHERE market_hash_name = ? AND appid = ? AND currency = ? ORDER BY recorded_at ASC"));
    q.addBindValue(marketHashName);
    q.addBindValue(appid);
    q.addBindValue(currency);
    if (!q.exec()) {
        qWarning() << "读取历史失败:" << q.lastError().text();
        return out;
    }
    while (q.next()) {
        PricePoint p;
        p.recordedAt = QDateTime::fromString(q.value(0).toString(), Qt::ISODate);
        p.price = q.value(1).toDouble();
        p.hasVolume = !q.value(2).isNull();
        p.volume = p.hasVolume ? q.value(2).toInt() : 0;
        out.append(p);
    }
    return out;
}
