#pragma once

#include <QVector>

#include "core/models/Alert.h"

class QSqlDatabase;
class QSqlQuery;

// alerts 表仓储。
class AlertRepository {
public:
    explicit AlertRepository(QSqlDatabase &db);

    QVector<Alert> all() const;
    QVector<Alert> enabled() const;
    bool add(const Alert &alert);
    bool update(const Alert &alert);
    bool remove(int id);
    bool markTriggered(int id, const QDateTime &at);

private:
    static Alert parse(const QSqlQuery &q, int offset);
    QSqlDatabase &m_db;
};
