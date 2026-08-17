#include "data/repositories/SettingsRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

SettingsRepository::SettingsRepository(QSqlDatabase &db) : m_db(db) {}

QString SettingsRepository::value(const QString &key, const QString &defaultValue) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key = ?"));
    q.addBindValue(key);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return defaultValue;
}

bool SettingsRepository::setValue(const QString &key, const QString &value) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO settings (key, value) VALUES (?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    q.addBindValue(key);
    q.addBindValue(value);
    if (!q.exec()) {
        qWarning() << "保存设置失败:" << q.lastError().text();
        return false;
    }
    return true;
}
