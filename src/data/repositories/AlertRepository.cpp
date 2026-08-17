#include "data/repositories/AlertRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

AlertRepository::AlertRepository(QSqlDatabase &db) : m_db(db) {}

Alert AlertRepository::parse(const QSqlQuery &q, int offset) {
    Alert a;
    a.id = q.value(offset).toInt();
    a.marketHashName = q.value(offset + 1).toString();
    a.appid = q.value(offset + 2).toInt();
    const QString type = q.value(offset + 3).toString();
    a.conditionType = type == QLatin1String("above")
                          ? Alert::Condition::kAbove
                          : (type == QLatin1String("percent_24h") ? Alert::Condition::kPercent24h
                                                                  : Alert::Condition::kBelow);
    a.thresholdValue = q.value(offset + 4).toDouble();
    a.percentValue = q.value(offset + 5).toDouble();
    a.enabled = q.value(offset + 6).toInt() != 0;
    const QString last = q.value(offset + 7).toString();
    if (!last.isEmpty()) a.lastTriggeredAt = QDateTime::fromString(last, Qt::ISODate);
    a.createdAt = QDateTime::fromString(q.value(offset + 8).toString(), Qt::ISODate);
    return a;
}

QVector<Alert> AlertRepository::all() const {
    QVector<Alert> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT id, market_hash_name, appid, condition_type, threshold_value, percent_value, "
            " enabled, last_triggered_at, created_at FROM alerts ORDER BY id"))) {
        qWarning() << "读取提醒失败:" << q.lastError().text();
        return out;
    }
    while (q.next()) out.append(parse(q, 0));
    return out;
}

QVector<Alert> AlertRepository::enabled() const {
    QVector<Alert> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT id, market_hash_name, appid, condition_type, threshold_value, percent_value, "
            " enabled, last_triggered_at, created_at FROM alerts WHERE enabled = 1 ORDER BY id"))) {
        return out;
    }
    while (q.next()) out.append(parse(q, 0));
    return out;
}

bool AlertRepository::add(const Alert &alert) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO alerts (market_hash_name, appid, condition_type, threshold_value, percent_value, "
        " enabled, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))"));
    q.addBindValue(alert.marketHashName);
    q.addBindValue(alert.appid);
    q.addBindValue(alert.conditionType == Alert::Condition::kAbove
                       ? QStringLiteral("above")
                       : (alert.conditionType == Alert::Condition::kPercent24h
                              ? QStringLiteral("percent_24h")
                              : QStringLiteral("below")));
    q.addBindValue(alert.thresholdValue > 0 ? QVariant(alert.thresholdValue) : QVariant());
    q.addBindValue(alert.percentValue > 0 ? QVariant(alert.percentValue) : QVariant());
    q.addBindValue(alert.enabled ? 1 : 0);
    if (!q.exec()) {
        qWarning() << "创建提醒失败:" << q.lastError().text();
        return false;
    }
    return true;
}

bool AlertRepository::update(const Alert &alert) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE alerts SET condition_type=?, threshold_value=?, percent_value=?, enabled=?, "
        " updated_at=datetime('now') WHERE id=?"));
    q.addBindValue(alert.conditionType == Alert::Condition::kAbove
                       ? QStringLiteral("above")
                       : (alert.conditionType == Alert::Condition::kPercent24h
                              ? QStringLiteral("percent_24h")
                              : QStringLiteral("below")));
    q.addBindValue(alert.thresholdValue > 0 ? QVariant(alert.thresholdValue) : QVariant());
    q.addBindValue(alert.percentValue > 0 ? QVariant(alert.percentValue) : QVariant());
    q.addBindValue(alert.enabled ? 1 : 0);
    q.addBindValue(alert.id);
    if (!q.exec()) {
        qWarning() << "更新提醒失败:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool AlertRepository::remove(int id) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM alerts WHERE id = ?"));
    q.addBindValue(id);
    return q.exec() && q.numRowsAffected() > 0;
}

bool AlertRepository::markTriggered(int id, const QDateTime &at) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE alerts SET last_triggered_at = ?, updated_at = datetime('now') WHERE id = ?"));
    q.addBindValue(at.toString(Qt::ISODate));
    q.addBindValue(id);
    return q.exec();
}
