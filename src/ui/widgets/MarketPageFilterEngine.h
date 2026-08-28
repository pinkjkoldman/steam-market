#pragma once

#include <QString>
#include <QVector>

#include "ui/widgets/WorkbenchModels.h"

struct MarketPageFilter {
    QString typeContains;
    qint64 minimumPriceMinor = 0;
    qint64 maximumPriceMinor = 0;
    qint64 minimumListings = 0;
    bool pricedOnly = false;
};

struct MarketPageFilterResult {
    QVector<MarketCatalogItemView> items;
    qint64 minimumPriceMinor = 0;
    qint64 maximumPriceMinor = 0;
    qint64 medianPriceMinor = 0;
    qint64 totalListings = 0;
};

class MarketPageFilterEngine {
public:
    static MarketPageFilterResult apply(const QVector<MarketCatalogItemView> &items,
                                        const MarketPageFilter &filter);
};
