#pragma once

#include <QVector>

#include "core/models/TradeRecord.h"

class QSqlDatabase;

// trades 表仓储（模拟交易）。
class TradeRepository {
public:
    explicit TradeRepository(QSqlDatabase &db);

    QVector<TradeRecord> all() const;
    bool add(const TradeRecord &record);
    bool remove(int id);
    // 某物品当前持仓数量（买入合计 - 卖出合计）。
    int holdings(const QString &marketHashName) const;

private:
    QSqlDatabase &m_db;
};
