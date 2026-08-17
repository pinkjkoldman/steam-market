#include "core/services/TradeSimulationService.h"

#include "core/services/PortfolioService.h"
#include "core/services/TradingRulesService.h"
#include "data/repositories/ItemRepository.h"
#include "data/repositories/TradeRepository.h"

TradeSimulationService::TradeSimulationService(TradeRepository *repo, ItemRepository *items,
                                               TradingRulesService *rules,
                                               PortfolioService *portfolio, QObject *parent)
    : QObject(parent), m_repo(repo), m_items(items), m_rules(rules), m_portfolio(portfolio) {}

QVector<TradeRecord> TradeSimulationService::records() const {
    return m_repo ? m_repo->all() : QVector<TradeRecord>();
}

QString TradeSimulationService::record(const QString &marketHashName, TradeRecord::Side side,
                                       int quantity, double price, const QString &note) {
    if (marketHashName.trimmed().isEmpty()) {
        return QStringLiteral("物品名称不能为空");
    }
    if (quantity < 1) {
        return QStringLiteral("数量必须 ≥ 1");
    }
    if (price <= 0) {
        return QStringLiteral("价格必须大于 0");
    }
    if (side == TradeRecord::Side::kSell && m_repo->holdings(marketHashName) < quantity) {
        return QStringLiteral("卖出数量超过当前模拟持仓");
    }

    MarketItem item;
    item.marketHashName = marketHashName;
    item.name = marketHashName;
    m_items->upsert(item);  // 确保外键存在

    TradeRecord record;
    record.marketHashName = marketHashName;
    record.side = side;
    record.quantity = quantity;
    record.price = price;
    record.tradedAt = QDateTime::currentDateTimeUtc();
    record.note = note.trimmed();
    const double amount = price * quantity;
    if (side == TradeRecord::Side::kBuy) {
        const FeeEstimate fee = m_rules->estimateFee(QStringLiteral("buy"), amount);
        record.fee = fee.totalFee;
        record.total = amount + fee.totalFee;  // 买入总成本
    } else {
        const FeeEstimate fee = m_rules->estimateFee(QStringLiteral("sell"), amount);
        record.fee = fee.totalFee;
        record.total = amount - fee.totalFee;  // 卖出净得
    }
    if (!m_repo->add(record)) {
        return QStringLiteral("写入交易记录失败");
    }
    emit tradesChanged();
    if (m_portfolio) {
        m_portfolio->refreshPrices();
    }
    return QString();
}

bool TradeSimulationService::remove(int id) {
    const bool ok = m_repo->remove(id);
    if (ok) {
        emit tradesChanged();
    }
    return ok;
}
