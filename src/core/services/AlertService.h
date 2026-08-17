#pragma once

#include <QObject>
#include <QVector>

#include "core/models/Alert.h"
#include "core/models/PriceOverview.h"

class AlertRepository;
class PriceRepository;

// 价格提醒服务：CRUD 与阈值检查（below/above/percent_24h），命中回调通知，30 分钟冷却去重。
class AlertService : public QObject {
    Q_OBJECT

public:
    AlertService(AlertRepository *repo, PriceRepository *prices, QObject *parent = nullptr);

    QVector<Alert> alerts() const;
    bool add(const Alert &alert);
    bool update(const Alert &alert);
    bool remove(int id);
    // 基于最新快照检查全部启用提醒。
    void checkAll();

signals:
    void alertsChanged();
    void alertTriggered(const QString &title, const QString &body);

private:
    bool meets(const Alert &alert, const PriceOverview &snap) const;
    AlertRepository *m_repo = nullptr;
    PriceRepository *m_prices = nullptr;
};
