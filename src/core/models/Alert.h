#pragma once

#include <QDateTime>
#include <QString>

// 价格提醒（对应 api-contract.yaml Alert）。
struct Alert {
    enum class Condition { kBelow, kAbove, kPercent24h };

    int id = 0;
    QString marketHashName;
    int appid = 730;
    Condition conditionType = Condition::kBelow;
    double thresholdValue = 0.0;  // below/above 价格阈值
    double percentValue = 0.0;    // percent_24h 涨跌幅阈值
    bool enabled = true;
    QDateTime lastTriggeredAt;
    QDateTime createdAt;
};
