#pragma once

#include <QPair>
#include <QQueue>
#include <QObject>
#include <QVector>
#include <functional>

#include "core/errors.h"
#include "core/models/MarketItem.h"
#include "core/models/Orderbook.h"
#include "core/models/PriceOverview.h"
#include "core/models/PricePoint.h"
#include "network/RateLimiter.h"

class QNetworkAccessManager;
class QNetworkReply;

using SearchCallback = std::function<void(QVector<MarketItem>, const AppError &)>;
using OverviewCallback = std::function<void(PriceOverview, const AppError &)>;
using HistoryCallback = std::function<void(QVector<PricePoint>, const AppError &)>;
using OrderbookCallback = std::function<void(Orderbook, const AppError &)>;

// Steam 社区市场公开接口客户端：search/render、priceoverview、pricehistory。
// 全部异步；内部限速串行派发，失败自动重试最多 2 次（指数退避）。
class SteamMarketClient : public QObject {
    Q_OBJECT

public:
    explicit SteamMarketClient(QNetworkAccessManager *nam, QObject *parent = nullptr);
    ~SteamMarketClient() override;

    void setCurrency(const QString &currencyCode);
    void setRequestIntervalMs(qint64 ms);

    void search(const QString &query, int appid, SearchCallback callback);
    void fetchOverview(const QString &marketHashName, int appid, OverviewCallback callback);
    void fetchHistory(const QString &marketHashName, int appid, HistoryCallback callback);
    void fetchOrderbook(const QString &marketHashName, int appid, OrderbookCallback callback);

private:
    struct PendingRequest {
        enum class Kind { kSearch, kOverview, kHistory, kOrderbook };
        Kind kind = Kind::kSearch;
        QString marketHashName;
        QString query;
        int appid = 730;
        int attempt = 0;
        SearchCallback searchCb;
        OverviewCallback overviewCb;
        HistoryCallback historyCb;
        OrderbookCallback orderbookCb;
    };

    void dispatchNext();
    void enqueue(PendingRequest req);
    void handleReply(QNetworkReply *reply, PendingRequest req);
    void retryOrFail(PendingRequest req, const AppError &err);

    QNetworkAccessManager *m_nam = nullptr;
    RateLimiter m_limiter;
    QString m_currency = QStringLiteral("CNY");
    QQueue<PendingRequest> m_queue;
    bool m_busy = false;
};
