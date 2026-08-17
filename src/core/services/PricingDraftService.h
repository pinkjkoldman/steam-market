#pragma once

#include <QVector>

#include "core/models/InventoryModels.h"

class PricingDraftService {
public:
    QVector<ListingDraftLine> createFixedPriceDraft(const QVector<InventoryGroup> &groups,
                                                    qint64 buyerPaysMinor,
                                                    QString *error) const;
};
