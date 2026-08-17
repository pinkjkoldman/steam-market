#pragma once

#include <QVector>

#include "core/models/WatchlistItem.h"

class QSqlDatabase;

// watchlist 表仓储。
class WatchlistRepository {
public:
    explicit WatchlistRepository(QSqlDatabase &db);

    QVector<WatchlistItem> all() const;
    bool add(const QString &marketHashName, int appid, const QString &note);
    bool remove(const QString &marketHashName);
    bool contains(const QString &marketHashName) const;
    bool setNote(const QString &marketHashName, const QString &note);

private:
    QSqlDatabase &m_db;
};
