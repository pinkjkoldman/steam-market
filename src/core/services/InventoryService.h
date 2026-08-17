#pragma once

#include <QObject>

#include "core/models/InventoryModels.h"

class InventoryRepository;
class SteamInventoryClient;

class InventoryService : public QObject {
    Q_OBJECT

public:
    InventoryService(InventoryRepository *repository, SteamInventoryClient *client,
                     QObject *parent = nullptr);

    bool isSyncing() const { return m_isSyncing; }
    QVector<InventoryGroup> groups(const QString &steamId, int appid,
                                   const QString &contextId) const;
    void sync(const QString &steamId, int appid, const QString &contextId);
    void cancel();

signals:
    void syncProgress(int pageCount, int assetCount);
    void syncCompleted(const QVector<InventoryGroup> &groups);
    void syncFailed(const QString &message);

private:
    void fetchNextPage();
    void finishWithError(const QString &message);

    InventoryRepository *m_repository = nullptr;
    SteamInventoryClient *m_client = nullptr;
    QString m_syncId;
    QString m_steamId;
    QString m_contextId;
    QString m_cursor;
    int m_appid = 0;
    int m_pageCount = 0;
    int m_assetCount = 0;
    bool m_isSyncing = false;
    bool m_cancelled = false;
};
