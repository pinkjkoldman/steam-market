#include "core/services/PortfolioService.h"

#include "data/repositories/ItemRepository.h"
#include "data/repositories/PortfolioRepository.h"
#include "data/repositories/PriceRepository.h"
#include "network/SteamMarketClient.h"
#include "utils/CurrencyProvider.h"

PortfolioService::PortfolioService(PortfolioRepository *repo, PriceRepository *prices,
                                   ItemRepository *items, QObject *parent)
    : QObject(parent), m_repo(repo), m_prices(prices), m_items(items) {}

QVector<PortfolioItem> PortfolioService::items() const {
    QVector<PortfolioItem> out = m_repo->all();
    for (PortfolioItem &it : out) {
        const PriceOverview snap =
            m_prices->latestSnapshot(it.marketHashName, it.appid, CurrencyProvider::code());
        it.hasPrice = snap.priceHigh > 0 || snap.priceLow > 0;
        if (it.hasPrice) {
            it.latestPrice = snap.priceHigh > 0 ? snap.priceHigh : snap.priceLow;
            it.marketValue = it.latestPrice * it.quantity;
            if (it.purchasePrice > 0) {
                it.profitLoss = (it.latestPrice - it.purchasePrice) * it.quantity;
                it.profitLossPercent = (it.latestPrice - it.purchasePrice) / it.purchasePrice * 100.0;
            }
        }
    }
    return out;
}

PortfolioSummary PortfolioService::summary() const {
    PortfolioSummary s;
    s.currency = CurrencyProvider::code();
    const QVector<PortfolioItem> list = items();
    for (const PortfolioItem &it : list) {
        ++s.itemCount;
        if (!it.hasPrice) {
            ++s.missingPriceCount;
            continue;
        }
        s.totalMarketValue += it.marketValue;
        if (it.purchasePrice > 0) {
            s.totalCost += it.purchasePrice * it.quantity;
        }
    }
    s.totalProfitLoss = s.totalMarketValue - s.totalCost;
    if (s.totalCost > 0) {
        s.totalProfitLossPercent = s.totalProfitLoss / s.totalCost * 100.0;
    }
    return s;
}

bool PortfolioService::add(const PortfolioItem &item) {
    const bool ok = m_repo->add(item);
    if (ok) emit portfolioChanged();
    return ok;
}

bool PortfolioService::update(const PortfolioItem &item) {
    const bool ok = m_repo->update(item);
    if (ok) emit portfolioChanged();
    return ok;
}

bool PortfolioService::remove(int id) {
    const bool ok = m_repo->remove(id);
    if (ok) emit portfolioChanged();
    return ok;
}

void PortfolioService::setClient(SteamMarketClient *client) {
    m_client = client;
}

void PortfolioService::refreshPrices() {
    const QVector<PortfolioItem> list = m_repo->all();
    if (list.isEmpty()) {
        emit portfolioChanged();
        return;
    }
    if (!m_client) {
        // 未注入网络客户端（如单元测试）：仅重算估值。
        qInfo() << "持仓价格刷新待处理物品数:" << list.size();
        emit portfolioChanged();
        return;
    }
    m_pendingRefresh = list.size();
    for (const PortfolioItem &it : list) {
        const QString name = it.marketHashName;
        const int appid = it.appid > 0 ? it.appid : 730;
        m_client->fetchOverview(name, appid,
                                [this, appid](PriceOverview overview, const AppError &err) {
            if (err.isOk() && (overview.priceLow > 0 || overview.priceHigh > 0)) {
                m_prices->saveSnapshot(overview, appid);
            }
            --m_pendingRefresh;
            if (m_pendingRefresh <= 0) {
                m_pendingRefresh = 0;
                emit portfolioChanged();
            }
        });
    }
}
