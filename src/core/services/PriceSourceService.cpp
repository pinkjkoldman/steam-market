#include "core/services/PriceSourceService.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

#include "data/repositories/PriceRepository.h"
#include "price_sources/CsvPriceSource.h"

PriceSourceService::PriceSourceService(QSqlDatabase &db, PriceRepository *prices, QObject *parent)
    : QObject(parent), m_db(db), m_prices(prices) {}

QVector<PlatformPrice> PriceSourceService::compare(const QString &marketHashName) const {
    QVector<PlatformPrice> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT platform, price, currency, url, updated_at FROM platform_prices "
        "WHERE market_hash_name = ? ORDER BY platform"));
    q.addBindValue(marketHashName);
    if (q.exec()) {
        while (q.next()) {
            PlatformPrice p;
            p.platform = q.value(0).toString();
            p.marketHashName = marketHashName;
            p.price = q.value(1).isNull() ? -1.0 : q.value(1).toDouble();
            p.currency = q.value(2).toString();
            p.url = q.value(3).toString();
            p.updatedAt = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
            p.hasPrice = p.price > 0;
            out.append(p);
        }
    }
    return out;
}

int PriceSourceService::importCsv(const QString &filePath, QString *errorMessage) {
    AppError err;
    CsvPriceSource source(m_db);
    const int rows = source.import(filePath, &err);
    if (rows < 0 && errorMessage) {
        *errorMessage = err.message;
    }
    if (rows >= 0) {
        emit compareUpdated(QString());
    }
    return rows;
}

QStringList PriceSourceService::availablePlatforms() const {
    QStringList out;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT DISTINCT platform FROM platform_prices ORDER BY platform"))) {
        while (q.next()) out << q.value(0).toString();
    }
    return out;
}
