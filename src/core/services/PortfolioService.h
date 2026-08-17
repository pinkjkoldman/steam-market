#pragma once

#include <QObject>
#include <QVector>

#include "core/models/PortfolioItem.h"

class PortfolioRepository;
class PriceRepository;
class ItemRepository;
class SteamMarketClient;

// 持仓服务：明细估值、增删改、汇总。
class PortfolioService : public QObject {
    Q_OBJECT

public:
    PortfolioService(PortfolioRepository *repo, PriceRepository *prices, ItemRepository *items,
                     QObject *parent = nullptr);

    QVector<PortfolioItem> items() const;
    PortfolioSummary summary() const;
    bool add(const PortfolioItem &item);
    bool update(const PortfolioItem &item);
    bool remove(int id);
    // 注入网络客户端后，refreshPrices 才会真实拉取最新价格；未注入时仅重算估值。
    void setClient(SteamMarketClient *client);
    void refreshPrices();

signals:
    void portfolioChanged();

private:
    PortfolioRepository *m_repo = nullptr;
    PriceRepository *m_prices = nullptr;
    ItemRepository *m_items = nullptr;
    SteamMarketClient *m_client = nullptr;
    int m_pendingRefresh = 0;
};
