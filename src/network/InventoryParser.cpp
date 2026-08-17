#include "network/InventoryParser.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {
QString jsonString(const QJsonValue &value) {
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toVariant().toLongLong());
    return value.toVariant().toString();
}

QString descriptionKey(const QString &classId, const QString &instanceId) {
    return classId + QLatin1Char(':') + (instanceId.isEmpty() ? QStringLiteral("0") : instanceId);
}
}  // namespace

InventoryPage InventoryParser::parse(const QByteArray &payload, int appid, AppError *error) {
    InventoryPage page;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = AppError::make(ErrorCode::kDataSourceUnavailable,
                                    QStringLiteral("Steam 库存响应不是有效 JSON"));
        }
        return page;
    }

    const QJsonObject root = document.object();
    if (root.contains(QStringLiteral("success"))
        && root.value(QStringLiteral("success")).toInt(1) != 1) {
        if (error) {
            *error = AppError::make(ErrorCode::kDataSourceUnavailable,
                                    QStringLiteral("Steam 拒绝了库存请求，库存可能是私密的"));
        }
        return page;
    }

    QHash<QString, InventoryDescription> descriptions;
    const QJsonArray descriptionArray = root.value(QStringLiteral("descriptions")).toArray();
    for (const QJsonValue &value : descriptionArray) {
        const QJsonObject object = value.toObject();
        InventoryDescription description;
        description.appid = appid;
        description.classId = jsonString(object.value(QStringLiteral("classid")));
        description.instanceId = jsonString(object.value(QStringLiteral("instanceid")));
        if (description.instanceId.isEmpty()) description.instanceId = QStringLiteral("0");
        description.marketHashName = object.value(QStringLiteral("market_hash_name")).toString();
        description.displayName = object.value(QStringLiteral("name")).toString();
        description.iconUrl = object.value(QStringLiteral("icon_url")).toString();
        description.itemType = object.value(QStringLiteral("type")).toString();
        description.marketable = object.value(QStringLiteral("marketable")).toInt() == 1;
        description.tradable = object.value(QStringLiteral("tradable")).toInt() == 1;
        const QJsonArray tags = object.value(QStringLiteral("tags")).toArray();
        description.category = categoryFromTags(tags);
        description.tagsJson = QString::fromUtf8(QJsonDocument(tags).toJson(QJsonDocument::Compact));
        descriptions.insert(descriptionKey(description.classId, description.instanceId),
                            description);
    }

    const QJsonArray assets = root.value(QStringLiteral("assets")).toArray();
    page.items.reserve(assets.size());
    for (const QJsonValue &value : assets) {
        const QJsonObject object = value.toObject();
        InventoryAsset asset;
        asset.assetId = jsonString(object.value(QStringLiteral("assetid")));
        asset.classId = jsonString(object.value(QStringLiteral("classid")));
        asset.instanceId = jsonString(object.value(QStringLiteral("instanceid")));
        if (asset.instanceId.isEmpty()) asset.instanceId = QStringLiteral("0");
        asset.amount = qMax(1, jsonString(object.value(QStringLiteral("amount"))).toInt());
        if (asset.assetId.isEmpty() || asset.classId.isEmpty()) continue;

        InventoryDescription description =
            descriptions.value(descriptionKey(asset.classId, asset.instanceId));
        if (description.classId.isEmpty()) {
            description.appid = appid;
            description.classId = asset.classId;
            description.instanceId = asset.instanceId;
            description.displayName = QStringLiteral("未识别物品 %1").arg(asset.classId);
        }
        page.items.append({asset, description});
    }

    page.hasMore = root.value(QStringLiteral("more_items")).toBool(false);
    page.lastAssetId = jsonString(root.value(QStringLiteral("last_assetid")));
    page.totalInventoryCount = root.value(QStringLiteral("total_inventory_count")).toInt();
    if (page.hasMore && page.lastAssetId.isEmpty()) {
        if (error) {
            *error = AppError::make(ErrorCode::kDataSourceUnavailable,
                                    QStringLiteral("Steam 分页响应缺少 last_assetid"));
        }
        return {};
    }
    if (error) *error = AppError::ok();
    return page;
}

QString InventoryParser::categoryFromTags(const QJsonArray &tags) {
    for (const QJsonValue &value : tags) {
        const QJsonObject tag = value.toObject();
        if (tag.value(QStringLiteral("category")).toString() != QLatin1String("item_class")) {
            continue;
        }
        const QString internal = tag.value(QStringLiteral("internal_name")).toString();
        if (internal == QLatin1String("item_class_2")) return QStringLiteral("trading_card");
        if (internal == QLatin1String("item_class_4")) return QStringLiteral("emoticon");
        if (internal == QLatin1String("item_class_5")) return QStringLiteral("profile_background");
        if (internal == QLatin1String("item_class_3")) return QStringLiteral("profile_background");
    }
    return QStringLiteral("unknown");
}
