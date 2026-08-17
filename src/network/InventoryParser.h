#pragma once

#include <QByteArray>
#include <QJsonArray>

#include "core/errors.h"
#include "core/models/InventoryModels.h"

class InventoryParser {
public:
    static InventoryPage parse(const QByteArray &payload, int appid, AppError *error);

private:
    static QString categoryFromTags(const QJsonArray &tags);
};
