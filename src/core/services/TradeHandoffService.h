#pragma once

#include <QString>
#include <QUrl>

#include "core/models/InventoryModels.h"

class TradeHandoffService {
public:
    QUrl createSingleListingUrl(const InventoryGroup &group,
                                const QString &sellerSteamId64,
                                QString *error) const;
    QUrl createTradeOfferUrl(const QString &partnerInput, QString *error) const;
};
