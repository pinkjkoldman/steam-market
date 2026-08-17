#pragma once

#include <QString>

#include "core/models/Orderbook.h"

class QSqlDatabase;

// orderbook_snapshots 仓储：每物品保留最新一份盘口。
class OrderbookRepository {
public:
    explicit OrderbookRepository(QSqlDatabase &db);

    bool save(const Orderbook &orderbook);
    Orderbook latest(const QString &marketHashName) const;

private:
    QSqlDatabase &m_db;
};
