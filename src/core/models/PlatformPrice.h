#pragma once

#include <QDateTime>
#include <QString>

// 平台价格（对应 api-contract.yaml PlatformPrice）。
struct PlatformPrice {
    QString platform;
    QString marketHashName;
    double price = -1.0;
    QString currency = QStringLiteral("CNY");
    QString url;
    QDateTime updatedAt;
    bool hasPrice = false;
};
