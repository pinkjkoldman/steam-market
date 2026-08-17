#pragma once

#include <QVector>

#include "core/models/PortfolioItem.h"

class QSqlDatabase;

// portfolio_items 表仓储。
class PortfolioRepository {
public:
    explicit PortfolioRepository(QSqlDatabase &db);

    QVector<PortfolioItem> all() const;
    bool add(const PortfolioItem &item);
    bool update(const PortfolioItem &item);
    bool remove(int id);
    QStringList marketHashNames() const;

private:
    QSqlDatabase &m_db;
};
