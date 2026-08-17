#pragma once

#include <QSqlDatabase>

#include "core/models/InventoryModels.h"

class InventoryRepository {
public:
    explicit InventoryRepository(QSqlDatabase &database);

    bool beginSync(const QString &syncId, const QString &steamId, int appid,
                   const QString &contextId, QString *error);
    bool savePage(const QString &syncId, const QString &steamId, int appid,
                  const QString &contextId, const InventoryPage &page, int pageCount,
                  int assetCount, QString *error);
    bool completeSync(const QString &syncId, const QString &steamId, int appid,
                      const QString &contextId, int pageCount, int assetCount, QString *error);
    void failSync(const QString &syncId, const QString &errorCode);
    QVector<InventoryGroup> groups(const QString &steamId, int appid,
                                   const QString &contextId, bool marketableOnly = true) const;

private:
    QSqlDatabase m_database;
};
