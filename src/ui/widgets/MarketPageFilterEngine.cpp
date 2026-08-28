#include "ui/widgets/MarketPageFilterEngine.h"

#include <algorithm>

MarketPageFilterResult MarketPageFilterEngine::apply(
    const QVector<MarketCatalogItemView> &items, const MarketPageFilter &filter) {
    MarketPageFilterResult result;
    QVector<qint64> prices;
    for (const MarketCatalogItemView &item : items) {
        if (!filter.typeContains.trimmed().isEmpty()
            && !item.typeText.contains(filter.typeContains.trimmed(), Qt::CaseInsensitive)) {
            continue;
        }
        if (filter.pricedOnly && item.lowestSellMinor <= 0) continue;
        if (filter.minimumPriceMinor > 0
            && item.lowestSellMinor < filter.minimumPriceMinor) {
            continue;
        }
        if (filter.maximumPriceMinor > 0
            && item.lowestSellMinor > filter.maximumPriceMinor) {
            continue;
        }
        if (filter.minimumListings > 0 && item.sellListings < filter.minimumListings) {
            continue;
        }
        result.items.append(item);
        result.totalListings += item.sellListings;
        if (item.lowestSellMinor > 0) prices.append(item.lowestSellMinor);
    }
    if (!prices.isEmpty()) {
        std::sort(prices.begin(), prices.end());
        result.minimumPriceMinor = prices.first();
        result.maximumPriceMinor = prices.last();
        const int middle = prices.size() / 2;
        result.medianPriceMinor = prices.size() % 2 == 0
                                      ? (prices.at(middle - 1) + prices.at(middle)) / 2
                                      : prices.at(middle);
    }
    return result;
}
