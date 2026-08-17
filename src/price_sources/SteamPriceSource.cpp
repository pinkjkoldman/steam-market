#include "price_sources/SteamPriceSource.h"

#include "data/repositories/PriceRepository.h"
#include "utils/CurrencyProvider.h"

SteamPriceSource::SteamPriceSource(PriceRepository *repo) : m_repo(repo) {}

QString SteamPriceSource::platform() const {
    return QStringLiteral("steam");
}

void SteamPriceSource::fetchPrice(
    const QString &marketHashName,
    std::function<void(std::optional<PlatformPrice>, const AppError &)> cb) {
    if (!m_repo) {
        cb(std::nullopt, AppError::make(ErrorCode::kInternal, QStringLiteral("Steam 数据源未初始化")));
        return;
    }
    const PriceOverview snap =
        m_repo->latestSnapshot(marketHashName, 730, CurrencyProvider::code());
    if (snap.priceHigh <= 0 && snap.priceLow <= 0) {
        cb(std::nullopt, AppError::make(ErrorCode::kNotFound, QStringLiteral("暂无 Steam 价格数据")));
        return;
    }
    PlatformPrice p;
    p.platform = platform();
    p.marketHashName = marketHashName;
    p.price = snap.priceHigh > 0 ? snap.priceHigh : snap.priceLow;
    p.currency = snap.currency;
    p.updatedAt = snap.updatedAt;
    p.hasPrice = true;
    cb(p, AppError::ok());
}
