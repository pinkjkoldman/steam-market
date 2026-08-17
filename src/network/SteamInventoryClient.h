#pragma once

#include <QObject>

#include <functional>

#include "core/errors.h"
#include "core/models/InventoryModels.h"

class QNetworkAccessManager;

using InventoryPageCallback = std::function<void(const InventoryPage &, const AppError &)>;

class SteamInventoryClient : public QObject {
    Q_OBJECT

public:
    explicit SteamInventoryClient(QNetworkAccessManager *network, QObject *parent = nullptr);

    void fetchPage(const QString &steamId, int appid, const QString &contextId,
                   const QString &startAssetId, InventoryPageCallback callback);

private:
    QNetworkAccessManager *m_network = nullptr;
};
