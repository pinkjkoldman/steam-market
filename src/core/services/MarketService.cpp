#include "core/services/MarketService.h"

#include "data/repositories/ItemRepository.h"
#include "data/repositories/PriceRepository.h"
#include "utils/CurrencyProvider.h"

namespace {
QString historyKey(const QString &marketHashName, int appid, const QString &currency) {
    return QStringLiteral("%1\n%2\n%3").arg(appid).arg(currency).arg(marketHashName);
}
}  // namespace

MarketService::MarketService(SteamMarketClient *client, ItemRepository *items,
                             PriceRepository *prices, QObject *parent)
    : QObject(parent), m_client(client), m_items(items), m_prices(prices) {}

void MarketService::search(const QString &query, int appid, int start) {
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) {
        emit searchFinished({}, -1, QStringLiteral("请输入搜索关键词"));
        return;
    }
    m_client->search(trimmed.left(128), appid,
                     [this](QVector<MarketItem> items, int totalCount, const AppError &err) {
                         if (err.isOk()) {
                             for (const MarketItem &item : items) {
                                 m_items->upsert(item);
                             }
                         }
                         emit searchFinished(items, totalCount, err.isOk() ? QString() : err.message);
                     },
                     start);
}

void MarketService::fetchDetail(const QString &marketHashName, int appid) {
    const PriceOverview cached = cachedOverview(marketHashName, appid);
    if (cached.updatedAt.isValid()) emit overviewUpdated(marketHashName, cached, QString());
    fetchOverview(marketHashName, appid);
    fetchHistory(marketHashName, appid);
}

void MarketService::refreshHistory(const QString &marketHashName, int appid) {
    fetchHistory(marketHashName, appid);
}

void MarketService::fetchHistory(const QString &marketHashName, int appid) {
    const QString currency = CurrencyProvider::code();
    const QVector<PricePoint> cached = m_prices->history(marketHashName, appid, currency);
    HistorySnapshot snapshot;
    snapshot.marketHashName = marketHashName;
    snapshot.appid = appid;
    snapshot.currency = currency;
    snapshot.points = cached;
    if (!cached.isEmpty()) snapshot.fetchedAt = cached.last().recordedAt;

    if (!m_historyAuthenticated) {
        snapshot.state = HistoryDataState::kAuthRequired;
        snapshot.message = QStringLiteral("登录 Steam 后可读取官方历史；当前仅显示本地缓存");
        emit historyUpdated(snapshot);
        return;
    }

    const QString key = historyKey(marketHashName, appid, currency);
    if (m_historyInFlight.contains(key)) return;
    m_historyInFlight.insert(key);
    snapshot.state = HistoryDataState::kLoading;
    snapshot.message = cached.isEmpty() ? QStringLiteral("正在读取 Steam 官方历史…")
                                        : QStringLiteral("正在刷新；当前显示缓存历史");
    emit historyUpdated(snapshot);

    m_client->fetchHistory(marketHashName, appid,
                           [this, marketHashName, appid, currency, key](
                               QVector<PricePoint> points, const AppError &err) {
        m_historyInFlight.remove(key);
        HistorySnapshot result;
        result.marketHashName = marketHashName;
        result.appid = appid;
        result.currency = currency;
        const QVector<PricePoint> fallback =
            m_prices->history(marketHashName, appid, currency);
        if (err.isOk()) {
            if (points.isEmpty()) {
                result.points = fallback;
                result.state = fallback.isEmpty() ? HistoryDataState::kEmpty
                                                   : HistoryDataState::kCached;
                result.message = fallback.isEmpty()
                                     ? QStringLiteral("Steam 本次未返回历史点")
                                     : QStringLiteral("Steam 本次未返回历史点，继续显示缓存");
                emit historyUpdated(result);
                return;
            }
            result.persisted = m_prices->saveHistory(marketHashName, appid, currency, points);
            result.points = result.persisted
                                ? m_prices->history(marketHashName, appid, currency)
                                : points;
            result.state = HistoryDataState::kSteamLive;
            result.fetchedAt = QDateTime::currentDateTimeUtc();
            result.message = result.persisted
                                 ? QStringLiteral("Steam 官方历史已更新")
                                 : QStringLiteral("Steam 历史已获取，但本地缓存保存失败");
            emit historyUpdated(result);
            return;
        }

        result.points = fallback;
        result.message = err.message;
        if (err.code == ErrorCode::kAuthenticationRequired) {
            result.state = HistoryDataState::kAuthRequired;
        } else if (err.code == ErrorCode::kSourceInvalid) {
            result.state = HistoryDataState::kInvalidResponse;
        } else if (!fallback.isEmpty()) {
            result.state = HistoryDataState::kCached;
        } else if (err.code == ErrorCode::kRateLimited) {
            result.state = HistoryDataState::kRateLimited;
        } else {
            result.state = HistoryDataState::kUnavailable;
        }
        if (err.code == ErrorCode::kAuthenticationRequired) {
            m_historyAuthenticated = false;
            emit historyAuthenticationExpired();
        }
        emit historyUpdated(result);
    });
}

void MarketService::fetchOverview(const QString &marketHashName, int appid) {
    m_client->fetchOverview(marketHashName, appid,
                            [this, marketHashName, appid](PriceOverview overview,
                                                          const AppError &err) {
        if (err.isOk() && (overview.priceLow > 0 || overview.priceHigh > 0)) {
            m_prices->saveSnapshot(overview, appid);
        }
        emit overviewUpdated(marketHashName,
                             err.isOk() ? overview : cachedOverview(marketHashName, appid),
                             err.message);
    });
}

PriceOverview MarketService::cachedOverview(const QString &marketHashName, int appid) const {
    return m_prices->latestSnapshot(marketHashName, appid, CurrencyProvider::code());
}

QVector<PricePoint> MarketService::cachedHistory(const QString &marketHashName, int appid) const {
    return m_prices->history(marketHashName, appid, CurrencyProvider::code());
}

QString MarketService::iconUrl(const QString &marketHashName) const {
    return m_items->iconUrlOf(marketHashName);
}
