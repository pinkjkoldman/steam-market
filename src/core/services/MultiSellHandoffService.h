#pragma once

#include <QVector>

#include "core/models/InventoryModels.h"

class MultiSellHandoffService {
public:
    QVector<HandoffBatch> createBatches(const QVector<ListingDraftLine> &lines,
                                        QString *error) const;
    bool openBatch(const HandoffBatch &batch) const;
};
