#pragma once

#include <QDate>
#include <QString>

// 交易规则条目（对应 api-contract.yaml TradingRule）。
struct TradingRule {
    QString id;
    QString category;
    QString title;
    QString content;
    QString source;
    QDate updatedAt;
};
