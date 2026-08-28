#include "data/repositories/InventoryRepository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
bool failWith(const QSqlQuery &query, QString *error) {
    if (error) *error = query.lastError().text();
    return false;
}
}

InventoryRepository::InventoryRepository(QSqlDatabase &database) : m_database(database) {}

bool InventoryRepository::beginSync(const QString &syncId, const QString &steamId, int appid,
                                    const QString &contextId, QString *error) {
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!m_database.transaction()) {
        if (error) *error = m_database.lastError().text();
        return false;
    }
    QSqlQuery account(m_database);
    account.prepare(QStringLiteral(
        "INSERT INTO steam_accounts(steam_id,display_name,session_state,last_verified_at,created_at,updated_at) "
        "VALUES(?,NULL,'authenticated',?,?,?) "
        "ON CONFLICT(steam_id) DO UPDATE SET session_state='authenticated',last_verified_at=excluded.last_verified_at,updated_at=excluded.updated_at"));
    account.addBindValue(steamId);
    account.addBindValue(now);
    account.addBindValue(now);
    account.addBindValue(now);
    if (!account.exec()) {
        m_database.rollback();
        return failWith(account, error);
    }
    QSqlQuery sync(m_database);
    sync.prepare(QStringLiteral(
        "INSERT INTO inventory_syncs(sync_id,steam_id,appid,context_id,state,started_at) "
        "VALUES(?,?,?,?,'running',?)"));
    sync.addBindValue(syncId);
    sync.addBindValue(steamId);
    sync.addBindValue(appid);
    sync.addBindValue(contextId);
    sync.addBindValue(now);
    if (!sync.exec()) {
        m_database.rollback();
        return failWith(sync, error);
    }
    return m_database.commit();
}

bool InventoryRepository::savePage(const QString &syncId, const QString &steamId, int appid,
                                   const QString &contextId, const InventoryPage &page,
                                   int pageCount, int assetCount, QString *error) {
    if (!m_database.transaction()) {
        if (error) *error = m_database.lastError().text();
        return false;
    }
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery description(m_database);
    description.prepare(QStringLiteral(
        "INSERT INTO inventory_descriptions(appid,class_id,instance_id,market_hash_name,display_name,icon_url,item_type,category,marketable,tradable,tags_json,updated_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(appid,class_id,instance_id) DO UPDATE SET "
        "market_hash_name=excluded.market_hash_name,display_name=excluded.display_name,icon_url=excluded.icon_url,item_type=excluded.item_type,category=excluded.category,marketable=excluded.marketable,tradable=excluded.tradable,tags_json=excluded.tags_json,updated_at=excluded.updated_at"));
    QSqlQuery asset(m_database);
    asset.prepare(QStringLiteral(
        "INSERT INTO inventory_assets(steam_id,appid,context_id,asset_id,class_id,instance_id,amount,last_seen_sync_id,last_seen_at) "
        "VALUES(?,?,?,?,?,?,?,?,?) ON CONFLICT(steam_id,appid,context_id,asset_id) DO UPDATE SET "
        "class_id=excluded.class_id,instance_id=excluded.instance_id,amount=excluded.amount,last_seen_sync_id=excluded.last_seen_sync_id,last_seen_at=excluded.last_seen_at"));
    for (const InventoryItem &item : page.items) {
        const InventoryDescription &d = item.description;
        description.bindValue(0, appid);
        description.bindValue(1, d.classId);
        description.bindValue(2, d.instanceId);
        description.bindValue(3, d.marketHashName.isEmpty() ? QVariant() : QVariant(d.marketHashName));
        description.bindValue(4, d.displayName);
        description.bindValue(5, d.iconUrl);
        description.bindValue(6, d.itemType);
        description.bindValue(7, d.category);
        description.bindValue(8, d.marketable ? 1 : 0);
        description.bindValue(9, d.tradable ? 1 : 0);
        description.bindValue(10, d.tagsJson);
        description.bindValue(11, now);
        if (!description.exec()) {
            m_database.rollback();
            return failWith(description, error);
        }
        const InventoryAsset &a = item.asset;
        asset.bindValue(0, steamId);
        asset.bindValue(1, appid);
        asset.bindValue(2, contextId);
        asset.bindValue(3, a.assetId);
        asset.bindValue(4, a.classId);
        asset.bindValue(5, a.instanceId);
        asset.bindValue(6, a.amount);
        asset.bindValue(7, syncId);
        asset.bindValue(8, now);
        if (!asset.exec()) {
            m_database.rollback();
            return failWith(asset, error);
        }
    }
    QSqlQuery update(m_database);
    update.prepare(QStringLiteral(
        "UPDATE inventory_syncs SET cursor=?,page_count=?,asset_count=? WHERE sync_id=? AND state='running'"));
    update.addBindValue(page.lastAssetId);
    update.addBindValue(pageCount);
    update.addBindValue(assetCount);
    update.addBindValue(syncId);
    if (!update.exec()) {
        m_database.rollback();
        return failWith(update, error);
    }
    return m_database.commit();
}

bool InventoryRepository::completeSync(const QString &syncId, const QString &steamId, int appid,
                                       const QString &contextId, int pageCount, int assetCount,
                                       QString *error) {
    if (!m_database.transaction()) {
        if (error) *error = m_database.lastError().text();
        return false;
    }
    QSqlQuery stale(m_database);
    stale.prepare(QStringLiteral(
        "DELETE FROM inventory_assets WHERE steam_id=? AND appid=? AND context_id=? AND last_seen_sync_id<>?"));
    stale.addBindValue(steamId);
    stale.addBindValue(appid);
    stale.addBindValue(contextId);
    stale.addBindValue(syncId);
    if (!stale.exec()) {
        m_database.rollback();
        return failWith(stale, error);
    }
    QSqlQuery sync(m_database);
    sync.prepare(QStringLiteral(
        "UPDATE inventory_syncs SET state='completed',page_count=?,asset_count=?,completed_at=? WHERE sync_id=?"));
    sync.addBindValue(pageCount);
    sync.addBindValue(assetCount);
    sync.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    sync.addBindValue(syncId);
    if (!sync.exec()) {
        m_database.rollback();
        return failWith(sync, error);
    }
    return m_database.commit();
}

void InventoryRepository::failSync(const QString &syncId, const QString &errorCode) {
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE inventory_syncs SET state='failed',error_code=?,completed_at=? WHERE sync_id=?"));
    query.addBindValue(errorCode.left(120));
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(syncId);
    query.exec();
}

QVector<InventoryGroup> InventoryRepository::groups(const QString &steamId, int appid,
                                                    const QString &contextId,
                                                    bool marketableOnly) const {
    QVector<InventoryGroup> result;
    QSqlQuery query(m_database);
    QString sql = QStringLiteral(
        "SELECT d.market_hash_name,d.display_name,d.category,d.icon_url,SUM(a.amount),GROUP_CONCAT(a.asset_id,'|'),MIN(d.marketable),MIN(d.tradable) "
        "FROM inventory_assets a JOIN inventory_descriptions d ON d.appid=a.appid AND d.class_id=a.class_id AND d.instance_id=a.instance_id "
        "WHERE a.steam_id=? AND a.appid=? AND a.context_id=?");
    if (marketableOnly) sql += QStringLiteral(" AND d.marketable=1 AND d.market_hash_name IS NOT NULL");
    sql += QStringLiteral(
        " GROUP BY d.market_hash_name,d.display_name,d.category,d.icon_url "
        "ORDER BY d.category,d.display_name,d.market_hash_name");
    query.prepare(sql);
    query.addBindValue(steamId);
    query.addBindValue(appid);
    query.addBindValue(contextId);
    if (!query.exec()) return result;
    while (query.next()) {
        InventoryGroup group;
        group.appid = appid;
        group.contextId = contextId;
        group.marketHashName = query.value(0).toString();
        group.displayName = query.value(1).toString();
        group.category = query.value(2).toString();
        group.iconUrl = query.value(3).toString();
        group.inventoryQuantity = query.value(4).toInt();
        group.assetIds = query.value(5).toString().split(QLatin1Char('|'), Qt::SkipEmptyParts);
        group.marketable = query.value(6).toInt() == 1;
        group.tradable = query.value(7).toInt() == 1;
        result.append(group);
    }
    return result;
}
