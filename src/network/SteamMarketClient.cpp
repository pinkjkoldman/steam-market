#include "network/SteamMarketClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQueue>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QtDebug>

#include "network/SteamHistoryParser.h"
#include "network/SteamOrderbookParser.h"
#include "utils/Currency.h"

namespace {
const QString kSteamHost = QStringLiteral("https://steamcommunity.com");
const int kMaxAttempts = 2;

double parsePriceText(const QString &text) {
    // Steam 返回形如 "¥ 98.50" 或 "$98.50"；取首个数字起。
    int start = -1;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i).isDigit() || text.at(i) == QLatin1Char('.')) {
            start = i;
            break;
        }
    }
    if (start < 0) return -1.0;
    QString num;
    for (int i = start; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c.isDigit() || c == QLatin1Char('.')) {
            num.append(c);
        } else {
            break;
        }
    }
    bool ok = false;
    const double v = num.toDouble(&ok);
    return ok ? v : -1.0;
}

// Steam sale_count_text 形如 "1,234 listings"/"1,234 件在售"；
// 去除千分位逗号与空白后提取整数，直接 toInt 会因前缀/逗号解析为 0。
int parseVolumeText(const QString &text) {
    QString digits;
    for (const QChar &c : text) {
        if (c.isDigit()) digits.append(c);
    }
    bool ok = false;
    const int v = digits.toInt(&ok);
    return ok ? v : 0;
}

}  // namespace

SteamMarketClient::SteamMarketClient(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent), m_nam(nam), m_limiter(1500) {}

SteamMarketClient::~SteamMarketClient() = default;

void SteamMarketClient::setCurrency(const QString &currencyCode) {
    m_currency = currencyCode;
}

void SteamMarketClient::setRequestIntervalMs(qint64 ms) {
    m_limiter.setMinIntervalMs(ms);
}

void SteamMarketClient::search(const QString &query, int appid, SearchCallback callback,
                               int start) {
    PendingRequest req;
    req.kind = PendingRequest::Kind::kSearch;
    req.query = query;
    req.appid = appid;
    req.start = qMax(0, start);
    req.searchCb = std::move(callback);
    enqueue(std::move(req));
}

void SteamMarketClient::fetchOverview(const QString &marketHashName, int appid,
                                      OverviewCallback callback) {
    PendingRequest req;
    req.kind = PendingRequest::Kind::kOverview;
    req.marketHashName = marketHashName;
    req.appid = appid;
    req.overviewCb = std::move(callback);
    enqueue(std::move(req));
}

void SteamMarketClient::fetchHistory(const QString &marketHashName, int appid,
                                     HistoryCallback callback) {
    PendingRequest req;
    req.kind = PendingRequest::Kind::kHistory;
    req.marketHashName = marketHashName;
    req.appid = appid;
    req.historyCb = std::move(callback);
    enqueue(std::move(req));
}

void SteamMarketClient::fetchOrderbook(const QString &marketHashName, int appid,
                                       OrderbookCallback callback) {
    PendingRequest req;
    req.kind = PendingRequest::Kind::kOrderbook;
    req.marketHashName = marketHashName;
    req.appid = appid;
    req.orderbookCb = std::move(callback);
    enqueue(std::move(req));
}

void SteamMarketClient::enqueue(PendingRequest req) {
    m_queue.enqueue(std::move(req));
    dispatchNext();
}

void SteamMarketClient::dispatchNext() {
    if (m_busy || m_queue.isEmpty()) {
        return;
    }
    const qint64 wait = m_limiter.waitMs();
    if (wait > 0) {
        QTimer::singleShot(wait, this, [this]() { dispatchNext(); });
        return;
    }
    m_busy = true;
    m_limiter.tick();

    PendingRequest req = m_queue.dequeue();
    QNetworkRequest request;
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent",
                         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) SteamMarketTerminal/1.0");
    request.setTransferTimeout(15000);

    QUrl url(kSteamHost);
    QUrlQuery q;
    if (req.kind == PendingRequest::Kind::kSearch) {
        url.setPath(QStringLiteral("/market/search/render/"));
        q.addQueryItem(QStringLiteral("query"), req.query);
        q.addQueryItem(QStringLiteral("appid"), QString::number(req.appid));
        q.addQueryItem(QStringLiteral("norender"), QStringLiteral("1"));
        q.addQueryItem(QStringLiteral("count"), QStringLiteral("30"));
        if (req.start > 0) {
            q.addQueryItem(QStringLiteral("start"), QString::number(req.start));
        }
    } else if (req.kind == PendingRequest::Kind::kOverview) {
        url.setPath(QStringLiteral("/market/priceoverview/"));
        q.addQueryItem(QStringLiteral("appid"), QString::number(req.appid));
        q.addQueryItem(QStringLiteral("currency"), QString::number(Currency::steamId(m_currency)));
        q.addQueryItem(QStringLiteral("market_hash_name"), req.marketHashName);
    } else if (req.kind == PendingRequest::Kind::kHistory) {
        url.setPath(QStringLiteral("/market/pricehistory/"));
        q.addQueryItem(QStringLiteral("appid"), QString::number(req.appid));
        q.addQueryItem(QStringLiteral("currency"), QString::number(Currency::steamId(m_currency)));
        q.addQueryItem(QStringLiteral("market_hash_name"), req.marketHashName);
    } else {
        // Steam 2026 新版市场已将盘口迁移到 queryAction 协议；
        // 旧 itemordershistogram 不再接受 appid + market_hash_name。
        url.setPath(QStringLiteral("/market/orderbook"));
        q.addQueryItem(QStringLiteral("q"), QStringLiteral("Load"));
        QJsonArray parameters;
        parameters.append(req.appid);
        parameters.append(req.marketHashName);
        q.addQueryItem(QStringLiteral("qp"),
                       QString::fromUtf8(QJsonDocument(parameters).toJson(
                           QJsonDocument::Compact)));
        request.setRawHeader("x-valve-request-type", "queryAction");
    }
    url.setQuery(q);
    request.setUrl(url);

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, req]() {
        handleReply(reply, req);
    });
}

void SteamMarketClient::handleReply(QNetworkReply *reply, PendingRequest req) {
    reply->deleteLater();
    m_busy = false;

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        AppError err;
        if (status == 429) {
            err = AppError::make(ErrorCode::kRateLimited,
                                 QStringLiteral("Steam 请求过于频繁，请稍后重试"));
        } else if (req.kind == PendingRequest::Kind::kHistory
                   && (status == 400 || status == 401)) {
            err = AppError::make(ErrorCode::kAuthenticationRequired,
                                 QStringLiteral("Steam 官方历史需要有效登录会话"));
        } else if (status == 403) {
            err = AppError::make(ErrorCode::kRateLimited,
                                 QStringLiteral("Steam 拒绝访问，请稍后重试"));
        } else {
            err = AppError::make(ErrorCode::kNetworkError,
                                 QStringLiteral("Steam 接口请求失败：%1")
                                     .arg(reply->errorString()));
        }
        retryOrFail(std::move(req), err);
        return;
    }

    if (req.kind == PendingRequest::Kind::kHistory) {
        const QString contentType =
            reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
        if (!contentType.isEmpty() && !contentType.contains(QStringLiteral("json"))) {
            retryOrFail(std::move(req),
                        AppError::make(ErrorCode::kAuthenticationRequired,
                                       QStringLiteral("Steam 返回了登录页面，请重新登录")));
            return;
        }
        const SteamHistoryParseResult parsed = SteamHistoryParser::parse(body);
        if (!parsed.error.isOk()) {
            retryOrFail(std::move(req), parsed.error);
            return;
        }
        if (req.historyCb) req.historyCb(parsed.points, AppError::ok());
        dispatchNext();
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        retryOrFail(std::move(req),
                    AppError::make(ErrorCode::kInternal, QStringLiteral("Steam 响应解析失败")));
        return;
    }
    const QJsonObject root = doc.object();

    if (req.kind == PendingRequest::Kind::kOrderbook) {
        const SteamOrderbookParseResult parsed =
            SteamOrderbookParser::parseQueryAction(body, req.marketHashName);
        if (!parsed.error.isOk()) {
            retryOrFail(std::move(req), parsed.error);
            return;
        }
        if (req.orderbookCb) req.orderbookCb(parsed.orderbook, AppError::ok());
        dispatchNext();
        return;
    }

    if (req.kind == PendingRequest::Kind::kSearch) {
        QVector<MarketItem> items;
        const QJsonArray results = root.value(QStringLiteral("results")).toArray();
        for (const QJsonValue &v : results) {
            const QJsonObject o = v.toObject();
            MarketItem item;
            item.marketHashName = o.value(QStringLiteral("hash_name")).toString();
            item.name = o.value(QStringLiteral("name")).toString();
            item.appid = o.value(QStringLiteral("appid")).toInt(730);
            item.iconUrl = o.value(QStringLiteral("asset_description"))
                               .toObject()
                               .value(QStringLiteral("icon_url"))
                               .toString();
            item.price = parsePriceText(o.value(QStringLiteral("sale_price_text")).toString());
            item.hasPrice = item.price > 0;
            item.volume = parseVolumeText(o.value(QStringLiteral("sale_count_text")).toString());
            if (!item.marketHashName.isEmpty()) {
                items.append(item);
            }
        }
        // Steam 返回 total_count 用于分页（“加载更多”依据）。
        const int totalCount = root.value(QStringLiteral("total_count")).toInt(items.size());
        if (req.searchCb) req.searchCb(items, totalCount, AppError::ok());
    } else if (req.kind == PendingRequest::Kind::kOverview) {
        PriceOverview overview;
        overview.marketHashName = req.marketHashName;
        overview.currency = m_currency;
        overview.updatedAt = QDateTime::currentDateTimeUtc();
        if (root.value(QStringLiteral("success")).toBool(false)) {
            overview.priceLow = parsePriceText(root.value(QStringLiteral("lowest_price")).toString());
            overview.priceHigh = parsePriceText(root.value(QStringLiteral("median_price")).toString());
            overview.volume = root.value(QStringLiteral("volume")).toString().toInt();
        }
        if (req.overviewCb) req.overviewCb(overview, AppError::ok());
    }
    dispatchNext();
}

void SteamMarketClient::retryOrFail(PendingRequest req, const AppError &err) {
    const bool retryable = err.code == ErrorCode::kNetworkError;
    if (retryable && ++req.attempt <= kMaxAttempts) {
        const int delayMs = 500 * req.attempt;
        QTimer::singleShot(delayMs, this, [this, req]() {
            m_queue.prepend(req);
            dispatchNext();
        });
        return;
    }
    if (req.kind == PendingRequest::Kind::kSearch && req.searchCb) {
        req.searchCb({}, -1, err);
    } else if (req.kind == PendingRequest::Kind::kOverview && req.overviewCb) {
        req.overviewCb({}, err);
    } else if (req.kind == PendingRequest::Kind::kHistory && req.historyCb) {
        req.historyCb({}, err);
    } else if (req.kind == PendingRequest::Kind::kOrderbook && req.orderbookCb) {
        req.orderbookCb({}, err);
    }
    dispatchNext();
}
