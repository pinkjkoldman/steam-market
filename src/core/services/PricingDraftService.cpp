#include "core/services/PricingDraftService.h"

QVector<ListingDraftLine> PricingDraftService::createFixedPriceDraft(
    const QVector<InventoryGroup> &groups, qint64 buyerPaysMinor, QString *error) const {
    QVector<ListingDraftLine> lines;
    if (buyerPaysMinor <= 0) {
        if (error) *error = QStringLiteral("买家支付价格必须大于 0");
        return lines;
    }
    for (const InventoryGroup &group : groups) {
        if (group.selectedQuantity <= 0) continue;
        if (group.marketHashName.isEmpty() || group.selectedQuantity > group.inventoryQuantity
            || group.selectedQuantity > group.assetIds.size()) {
            if (error) *error = QStringLiteral("物品 %1 的选择数量无效").arg(group.displayName);
            return {};
        }
        ListingDraftLine line;
        line.group = group;
        line.buyerPaysMinor = buyerPaysMinor;
        line.sellerReceivesMinor = (buyerPaysMinor * 10000) / 11500;
        line.feeMinor = buyerPaysMinor - line.sellerReceivesMinor;
        lines.append(line);
    }
    if (lines.isEmpty() && error) *error = QStringLiteral("请至少选择一个物品");
    if (!lines.isEmpty() && error) error->clear();
    return lines;
}
