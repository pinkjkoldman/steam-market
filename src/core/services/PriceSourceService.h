#pragma once

#include <QObject>
#include <QVector>

#include "core/models/PlatformPrice.h"

class QSqlDatabase;
class PriceRepository;

// 比价服务：聚合 Steam 与平台价格；CSV 导入兜底。
class PriceSourceService : public QObject {
    Q_OBJECT

public:
    PriceSourceService(QSqlDatabase &db, PriceRepository *prices, QObject *parent = nullptr);

    QVector<PlatformPrice> compare(const QString &marketHashName) const;
    int importCsv(const QString &filePath, QString *errorMessage);
    QStringList availablePlatforms() const;

signals:
    void compareUpdated(const QString &marketHashName);

private:
    QSqlDatabase &m_db;
    PriceRepository *m_prices = nullptr;
};
