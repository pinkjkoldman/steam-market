#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

struct InventoryDescription {
    int appid = 0;
    QString classId;
    QString instanceId;
    QString marketHashName;
    QString displayName;
    QString iconUrl;
    QString itemType;
    QString category = QStringLiteral("unknown");
    bool marketable = false;
    bool tradable = false;
    QString tagsJson = QStringLiteral("[]");
};

struct InventoryAsset {
    QString assetId;
    QString classId;
    QString instanceId;
    int amount = 1;
};

struct InventoryItem {
    InventoryAsset asset;
    InventoryDescription description;
};

struct InventoryPage {
    QVector<InventoryItem> items;
    bool hasMore = false;
    QString lastAssetId;
    int totalInventoryCount = 0;
};

struct InventoryGroup {
    int appid = 0;
    QString contextId;
    QString marketHashName;
    QString displayName;
    QString category;
    QString iconUrl;
    QStringList assetIds;
    int inventoryQuantity = 0;
    int selectedQuantity = 0;
};

struct ListingDraftLine {
    InventoryGroup group;
    qint64 buyerPaysMinor = 0;
    qint64 feeMinor = 0;
    qint64 sellerReceivesMinor = 0;
};

struct HandoffBatch {
    int batchNo = 0;
    int appid = 0;
    QString contextId;
    int groupCount = 0;
    QUrl officialUrl;
};
