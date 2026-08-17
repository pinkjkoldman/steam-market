#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <functional>
#include <optional>

#include "core/models/MarketCatalog.h"

class QNetworkAccessManager;
class QNetworkReply;

using MarketCatalogClientCallback =
    std::function<void(const MarketCatalogPage &, const MarketCatalogError &)>;

// Steam Community Market public-page catalog adapter. It owns an isolated anonymous
// network context, permits one request at a time, and never performs automatic pagination.
class SteamMarketCatalogClient : public QObject {
    Q_OBJECT

public:
    explicit SteamMarketCatalogClient(QObject *parent = nullptr);
    ~SteamMarketCatalogClient() override;

    void fetchPage(const MarketCatalogQuery &query, MarketCatalogClientCallback callback);
    void cancelCurrent();
    void setMinimumIntervalMs(qint64 intervalMs);

    static MarketCatalogError validateQuery(const MarketCatalogQuery &query);
    static qint64 retryAfterDelayMs(const QByteArray &value);
    static QUrl requestUrl(const MarketCatalogQuery &query);
    static bool parseResponse(const QByteArray &payload, const MarketCatalogQuery &query,
                              const QDateTime &fetchedAt, MarketCatalogPage *page,
                              MarketCatalogError *error);

private:
    struct ActiveRequest {
        quint64 id = 0;
        MarketCatalogQuery query;
        MarketCatalogClientCallback callback;
        int retryCount = 0;
    };

    void scheduleDispatch(qint64 extraDelayMs = 0);
    void dispatch();
    void handleReply(QNetworkReply *reply, quint64 requestId);
    void retryOrFinish(const MarketCatalogError &error);
    void finish(const MarketCatalogPage &page, const MarketCatalogError &error);

    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    QTimer m_dispatchTimer;
    QElapsedTimer m_lastDispatch;
    bool m_hasDispatched = false;
    qint64 m_minimumIntervalMs = 1500;
    QDateTime m_rateLimitedUntil;
    quint64 m_nextRequestId = 1;
    std::optional<ActiveRequest> m_active;
};
