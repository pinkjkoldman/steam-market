#pragma once

#include <QString>

// 费用估算（对应 api-contract.yaml FeeEstimate）。
struct FeeEstimate {
    QString direction;  // buy / sell
    double inputPrice = 0.0;
    double steamFee = 0.0;
    double gameFee = 0.0;
    double totalFee = 0.0;
    double sellerReceives = 0.0;
    double buyerPays = 0.0;
    QString currency = QStringLiteral("CNY");
};
