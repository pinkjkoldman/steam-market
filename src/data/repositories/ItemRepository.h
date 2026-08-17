#pragma once

#include <QString>

#include "core/models/MarketItem.h"

class QSqlDatabase;

// items 表仓储：物品 upsert 与查询。
class ItemRepository {
public:
    explicit ItemRepository(QSqlDatabase &db);

    bool upsert(const MarketItem &item);
    QString iconUrlOf(const QString &marketHashName) const;

private:
    QSqlDatabase &m_db;
};
