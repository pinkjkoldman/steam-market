#include "core/services/InventoryService.h"

#include <QTimer>
#include <QUuid>

#include "data/repositories/InventoryRepository.h"
#include "network/SteamInventoryClient.h"

namespace {
constexpr int kMaxPages = 50;
constexpr int kPageIntervalMs = 4000;
}

InventoryService::InventoryService(InventoryRepository *repository,
                                   SteamInventoryClient *client, QObject *parent)
    : QObject(parent), m_repository(repository), m_client(client) {}

QVector<InventoryGroup> InventoryService::groups(const QString &steamId, int appid,
                                                 const QString &contextId) const {
    // 库存页需要同时展示不可交易/不可出售物品，选择与交接权限由表格状态约束。
    return m_repository->groups(steamId, appid, contextId, false);
}

void InventoryService::sync(const QString &steamId, int appid, const QString &contextId) {
    if (m_isSyncing) {
        emit syncFailed(QStringLiteral("已有库存同步正在进行"));
        return;
    }
    m_syncId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_steamId = steamId.trimmed();
    m_appid = appid;
    m_contextId = contextId.trimmed();
    m_cursor.clear();
    m_pageCount = 0;
    m_assetCount = 0;
    m_cancelled = false;
    QString error;
    if (!m_repository->beginSync(m_syncId, m_steamId, m_appid, m_contextId, &error)) {
        emit syncFailed(QStringLiteral("无法创建同步任务：%1").arg(error));
        return;
    }
    m_isSyncing = true;
    emit syncProgress(0, 0);
    fetchNextPage();
}

void InventoryService::cancel() {
    if (m_isSyncing) m_cancelled = true;
}

void InventoryService::fetchNextPage() {
    if (!m_isSyncing) return;
    if (m_cancelled) {
        finishWithError(QStringLiteral("库存同步已取消"));
        return;
    }
    m_client->fetchPage(
        m_steamId, m_appid, m_contextId, m_cursor,
        [this](const InventoryPage &page, const AppError &error) {
            if (!m_isSyncing) return;
            if (!error.isOk()) {
                finishWithError(error.message);
                return;
            }
            ++m_pageCount;
            m_assetCount += page.items.size();
            QString databaseError;
            if (!m_repository->savePage(m_syncId, m_steamId, m_appid, m_contextId,
                                        page, m_pageCount, m_assetCount, &databaseError)) {
                finishWithError(QStringLiteral("保存库存失败：%1").arg(databaseError));
                return;
            }
            emit syncProgress(m_pageCount, m_assetCount);
            if (page.hasMore) {
                if (m_pageCount >= kMaxPages || page.lastAssetId == m_cursor) {
                    finishWithError(QStringLiteral("Steam 库存分页超过安全上限或游标未推进"));
                    return;
                }
                m_cursor = page.lastAssetId;
                QTimer::singleShot(kPageIntervalMs, this, &InventoryService::fetchNextPage);
                return;
            }
            if (!m_repository->completeSync(m_syncId, m_steamId, m_appid, m_contextId,
                                            m_pageCount, m_assetCount, &databaseError)) {
                finishWithError(QStringLiteral("完成库存同步失败：%1").arg(databaseError));
                return;
            }
            m_isSyncing = false;
            emit syncCompleted(groups(m_steamId, m_appid, m_contextId));
        });
}

void InventoryService::finishWithError(const QString &message) {
    m_repository->failSync(m_syncId, message);
    m_isSyncing = false;
    emit syncFailed(message);
}
