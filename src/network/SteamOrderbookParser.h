#pragma once

#include <QByteArray>
#include <QString>

#include "core/errors.h"
#include "core/models/Orderbook.h"

struct SteamOrderbookParseResult {
    Orderbook orderbook;
    AppError error;
};

class SteamOrderbookParser {
public:
    static SteamOrderbookParseResult parseQueryAction(const QByteArray &body,
                                                       const QString &marketHashName);
};
