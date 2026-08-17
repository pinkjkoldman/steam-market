#pragma once

#include <QString>

class QSqlDatabase;

// settings 表仓储（key-value，值 JSON 编码）。
class SettingsRepository {
public:
    explicit SettingsRepository(QSqlDatabase &db);

    QString value(const QString &key, const QString &defaultValue = QString()) const;
    bool setValue(const QString &key, const QString &value);

private:
    QSqlDatabase &m_db;
};
