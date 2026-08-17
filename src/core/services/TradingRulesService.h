#pragma once

#include <QObject>
#include <QVector>

#include "core/models/FeeEstimate.h"
#include "core/models/TradingRule.h"

class SettingsService;

// Steam 交易规则库 + 费用计算服务（规则来自内嵌资源，费率取设置）。
class TradingRulesService : public QObject {
    Q_OBJECT

public:
    explicit TradingRulesService(SettingsService *settings, QObject *parent = nullptr);

    QVector<TradingRule> rules() const;
    QStringList categories() const;
    QVector<TradingRule> rulesByCategory(const QString &category) const;

    FeeEstimate estimateFee(const QString &direction, double price) const;

private:
    SettingsService *m_settings = nullptr;
    QVector<TradingRule> m_rules;
};
