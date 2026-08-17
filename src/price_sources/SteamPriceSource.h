#pragma once

#include <QString>

#include "core/models/PlatformPrice.h"
#include "price_sources/IPriceSource.h"

class PriceRepository;

// Steam 官方数据源：优先使用本地最新快照，保证比价区在离线时仍可展示。
class SteamPriceSource : public IPriceSource {
public:
    explicit SteamPriceSource(PriceRepository *repo);

    QString platform() const override;
    void fetchPrice(const QString &marketHashName,
                    std::function<void(std::optional<PlatformPrice>, const AppError &)> cb) override;

private:
    PriceRepository *m_repo = nullptr;
};
