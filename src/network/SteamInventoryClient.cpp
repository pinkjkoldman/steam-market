#include "network/SteamInventoryClient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include "network/InventoryParser.h"

namespace {
constexpr qsizetype kMaxResponseBytes = 32 * 1024 * 1024;
constexpr int kSteamInventoryPageSize = 2000;
}

SteamInventoryClient::SteamInventoryClient(QNetworkAccessManager *network, QObject *parent)
    : QObject(parent), m_network(network) {}

void SteamInventoryClient::fetchPage(const QString &steamId, int appid,
                                     const QString &contextId,
                                     const QString &startAssetId,
                                     InventoryPageCallback callback) {
    if (steamId.isEmpty() || appid <= 0 || contextId.isEmpty()) {
        callback({}, AppError::make(ErrorCode::kInvalidArgument,
                                    QStringLiteral("库存账号或上下文无效")));
        return;
    }
    QUrl url(QStringLiteral("https://steamcommunity.com/inventory/%1/%2/%3")
                 .arg(steamId, QString::number(appid), contextId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("l"), QStringLiteral("schinese"));
    query.addQueryItem(QStringLiteral("count"), QString::number(kSteamInventoryPageSize));
    if (!startAssetId.isEmpty()) {
        query.addQueryItem(QStringLiteral("start_assetid"), startAssetId);
    }
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.7");
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("SteamMarketTerminal/1.0"));
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, appid, callback]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        const auto replyError = reply->error();
        reply->deleteLater();
        if (status == 429) {
            callback({}, AppError::make(ErrorCode::kRateLimited,
                                        QStringLiteral("Steam 请求过于频繁，请稍后重试")));
            return;
        }
        if (status == 400) {
            callback({}, AppError::make(
                             ErrorCode::kDataSourceUnavailable,
                             QStringLiteral("Steam 拒绝了库存请求（HTTP 400），请稍后重试")));
            return;
        }
        if (status == 401 || status == 403) {
            callback({}, AppError::make(
                             ErrorCode::kDataSourceUnavailable,
                             QStringLiteral("该库存不可公开访问，或 Steam 登录会话已失效")));
            return;
        }
        if (replyError != QNetworkReply::NoError) {
            callback({}, AppError::make(ErrorCode::kNetworkError, networkError));
            return;
        }
        if (payload.size() > kMaxResponseBytes) {
            callback({}, AppError::make(ErrorCode::kDataSourceUnavailable,
                                        QStringLiteral("Steam 库存响应超过安全大小限制")));
            return;
        }
        AppError parseError;
        const InventoryPage page = InventoryParser::parse(payload, appid, &parseError);
        callback(page, parseError);
    });
}
