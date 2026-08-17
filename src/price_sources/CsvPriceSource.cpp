#include "price_sources/CsvPriceSource.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextStream>
#include <QVariant>
#include <QtDebug>

CsvPriceSource::CsvPriceSource(QSqlDatabase &db) : m_db(db) {}

int CsvPriceSource::import(const QString &filePath, AppError *error) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = AppError::make(ErrorCode::kInvalidArgument,
                                           QStringLiteral("无法打开文件：%1").arg(filePath));
        return -1;
    }
    QTextStream in(&f);
    int count = 0;
    bool first = true;
    m_db.transaction();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO platform_prices (market_hash_name, platform, price, currency, url, updated_at) "
        "VALUES (?, ?, ?, ?, ?, datetime('now')) "
        "ON CONFLICT(market_hash_name, platform) DO UPDATE SET "
        " price=excluded.price, currency=excluded.currency, url=excluded.url, updated_at=datetime('now')"));
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (first && line.contains(QStringLiteral("market_hash_name"))) {
            first = false;
            continue;  // 跳过表头
        }
        first = false;
        const QStringList cols = line.split(QLatin1Char(','));
        if (cols.size() < 3) continue;
        const QString hashName = cols.at(0).trimmed();
        const QString platformName =
            cols.at(1).trimmed().isEmpty() ? platform() : cols.at(1).trimmed();
        bool ok = false;
        const double price = cols.at(2).trimmed().toDouble(&ok);
        if (!ok || hashName.isEmpty()) continue;
        QSqlQuery ensureItem(m_db);
        ensureItem.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO items (market_hash_name, appid, name, first_seen_at, updated_at) "
            "VALUES (?, 730, ?, datetime('now'), datetime('now'))"));
        ensureItem.addBindValue(hashName);
        ensureItem.addBindValue(hashName);
        ensureItem.exec();
        q.addBindValue(hashName);
        q.addBindValue(platformName);
        q.addBindValue(price);
        q.addBindValue(cols.size() > 3 && !cols.at(3).trimmed().isEmpty() ? cols.at(3).trimmed()
                                                                          : QStringLiteral("CNY"));
        q.addBindValue(cols.size() > 4 ? cols.at(4).trimmed() : QString());
        if (q.exec()) ++count;
    }
    if (!m_db.commit()) {
        m_db.rollback();
        if (error) *error = AppError::make(ErrorCode::kInternal, QStringLiteral("CSV 导入落库失败"));
        return -1;
    }
    qInfo() << "CSV 导入成功" << count << "行";
    return count;
}
