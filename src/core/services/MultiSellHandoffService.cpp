#include "core/services/MultiSellHandoffService.h"

#include <QDesktopServices>
#include <QMap>
#include <QPair>
#include <QUrlQuery>

namespace {
constexpr int kMaxGroupsPerBatch = 40;
constexpr int kMaxEncodedUrlBytes = 1800;

QUrl buildUrl(int appid, const QString &contextId, const QStringList &names) {
    QUrl url(QStringLiteral("https://steamcommunity.com/market/multisell"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("appid"), QString::number(appid));
    query.addQueryItem(QStringLiteral("contextid"), contextId);
    for (const QString &name : names) query.addQueryItem(QStringLiteral("items[]"), name);
    url.setQuery(query);
    return url;
}
}  // namespace

QVector<HandoffBatch> MultiSellHandoffService::createBatches(
    const QVector<ListingDraftLine> &lines, QString *error) const {
    QMap<QPair<int, QString>, QStringList> groupedNames;
    for (const ListingDraftLine &line : lines) {
        if (line.group.selectedQuantity <= 0 || line.group.marketHashName.isEmpty()) continue;
        QStringList &names = groupedNames[{line.group.appid, line.group.contextId}];
        names.append(line.group.marketHashName);
    }
    QVector<HandoffBatch> batches;
    int batchNo = 1;
    for (auto iterator = groupedNames.cbegin(); iterator != groupedNames.cend(); ++iterator) {
        QStringList current;
        for (const QString &name : iterator.value()) {
            QStringList candidate = current;
            candidate.append(name);
            const QUrl candidateUrl = buildUrl(iterator.key().first, iterator.key().second,
                                               candidate);
            if (!current.isEmpty()
                && (candidate.size() > kMaxGroupsPerBatch
                    || candidateUrl.toEncoded().size() > kMaxEncodedUrlBytes)) {
                const QUrl url = buildUrl(iterator.key().first, iterator.key().second, current);
                batches.append({batchNo++, iterator.key().first, iterator.key().second,
                                static_cast<int>(current.size()), url});
                current = {name};
            } else {
                current = candidate;
            }
        }
        if (!current.isEmpty()) {
            const QUrl url = buildUrl(iterator.key().first, iterator.key().second, current);
            if (url.toEncoded().size() > kMaxEncodedUrlBytes) {
                if (error) *error = QStringLiteral("物品名称过长，无法安全生成 Steam 批量页面");
                return {};
            }
            batches.append({batchNo++, iterator.key().first, iterator.key().second,
                            static_cast<int>(current.size()), url});
        }
    }
    if (batches.isEmpty() && error) *error = QStringLiteral("没有可交接的上架草稿");
    if (!batches.isEmpty() && error) error->clear();
    return batches;
}

bool MultiSellHandoffService::openBatch(const HandoffBatch &batch) const {
    return batch.officialUrl.scheme() == QLatin1String("https")
           && batch.officialUrl.host() == QLatin1String("steamcommunity.com")
           && QDesktopServices::openUrl(batch.officialUrl);
}
