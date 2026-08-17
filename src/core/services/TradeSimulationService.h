#pragma once

#include <QObject>
#include <QVector>

#include "core/models/TradeRecord.h"

class TradeRepository;
class ItemRepository;
class TradingRulesService;
class PortfolioService;

// 模拟交易服务：记录买卖（自动计费），卖出校验持仓，联动持仓估值。
class TradeSimulationService : public QObject {
    Q_OBJECT

public:
    TradeSimulationService(TradeRepository *repo, ItemRepository *items,
                           TradingRulesService *rules, PortfolioService *portfolio,
                           QObject *parent = nullptr);

    QVector<TradeRecord> records() const;
    // 返回错误信息；成功返回空串。
    QString record(const QString &marketHashName, TradeRecord::Side side, int quantity,
                   double price, const QString &note = QString());
    bool remove(int id);

signals:
    void tradesChanged();

private:
    TradeRepository *m_repo = nullptr;
    ItemRepository *m_items = nullptr;
    TradingRulesService *m_rules = nullptr;
    PortfolioService *m_portfolio = nullptr;
};
