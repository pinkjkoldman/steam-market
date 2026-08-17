#pragma once

#include <QDate>
#include <QString>

// 持仓条目（对应 api-contract.yaml PortfolioItem）。
struct PortfolioItem {
    int id = 0;
    QString marketHashName;
    int appid = 730;
    int quantity = 1;
    double purchasePrice = -1.0;
    QString purchaseCurrency = QStringLiteral("CNY");
    QDate purchaseDate;
    QString note;
    double latestPrice = -1.0;
    double marketValue = -1.0;
    double profitLoss = 0.0;
    double profitLossPercent = 0.0;
    bool hasPrice = false;
};

// 持仓汇总（对应 api-contract.yaml PortfolioSummary）。
struct PortfolioSummary {
    QString currency;
    double totalMarketValue = 0.0;
    double totalCost = 0.0;
    double totalProfitLoss = 0.0;
    double totalProfitLossPercent = 0.0;
    int itemCount = 0;
    int missingPriceCount = 0;
};
