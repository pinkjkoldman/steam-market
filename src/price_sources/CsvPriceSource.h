#pragma once

#include <QString>

#include "core/errors.h"
#include "price_sources/IPriceSource.h"

class QSqlDatabase;

// CSV 价格导入兜底：将用户导出的第三方价格写入 platform_prices 表。
// 格式：market_hash_name,platform,price,currency,url（首行可为表头）。
class CsvPriceSource {
public:
    explicit CsvPriceSource(QSqlDatabase &db);

    QString platform() const { return QStringLiteral("csv"); }
    // 返回导入成功行数；失败返回 -1 并填充 error。
    int import(const QString &filePath, AppError *error);

private:
    QSqlDatabase &m_db;
};
