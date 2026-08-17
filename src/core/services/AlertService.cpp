#include "core/services/AlertService.h"

#include <QDateTime>
#include <QtDebug>

#include "data/repositories/AlertRepository.h"
#include "data/repositories/PriceRepository.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"

namespace {
const qint64 kCooldownMinutes = 30;
}  // namespace

AlertService::AlertService(AlertRepository *repo, PriceRepository *prices, QObject *parent)
    : QObject(parent), m_repo(repo), m_prices(prices) {}

QVector<Alert> AlertService::alerts() const {
    return m_repo ? m_repo->all() : QVector<Alert>();
}

bool AlertService::add(const Alert &alert) {
    const bool ok = m_repo->add(alert);
    if (ok) emit alertsChanged();
    return ok;
}

bool AlertService::update(const Alert &alert) {
    const bool ok = m_repo->update(alert);
    if (ok) emit alertsChanged();
    return ok;
}

bool AlertService::remove(int id) {
    const bool ok = m_repo->remove(id);
    if (ok) emit alertsChanged();
    return ok;
}

bool AlertService::meets(const Alert &alert, const PriceOverview &snap) const {
    if (!alert.enabled || (snap.priceHigh <= 0 && snap.priceLow <= 0)) return false;
    const double price = snap.priceHigh > 0 ? snap.priceHigh : snap.priceLow;
    if (alert.conditionType == Alert::Condition::kBelow) {
        return alert.thresholdValue > 0 && price <= alert.thresholdValue;
    }
    if (alert.conditionType == Alert::Condition::kAbove) {
        return alert.thresholdValue > 0 && price >= alert.thresholdValue;
    }
    if (alert.percentValue <= 0) return false;
    const double past = m_prices->priceAtOrBefore(
        alert.marketHashName, alert.appid, CurrencyProvider::code(), snap.updatedAt.addDays(-1));
    if (past <= 0) return false;
    return (price - past) / past * 100.0 >= alert.percentValue;
}

void AlertService::checkAll() {
    const QVector<Alert> list = m_repo->enabled();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const Alert &alert : list) {
        const PriceOverview snap =
            m_prices->latestSnapshot(alert.marketHashName, alert.appid, CurrencyProvider::code());
        if (!meets(alert, snap)) continue;
        if (alert.lastTriggeredAt.isValid()
            && alert.lastTriggeredAt.secsTo(now) < kCooldownMinutes * 60) {
            continue;  // 冷却期内不重复通知
        }
        QString text;
        const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
        const double price = snap.priceHigh > 0 ? snap.priceHigh : snap.priceLow;
        if (alert.conditionType == Alert::Condition::kBelow) {
            text = QStringLiteral("%1 已跌至 %2%3（提醒阈值 %2%4）")
                       .arg(alert.marketHashName)
                       .arg(symbol)
                       .arg(price, 0, 'f', 2)
                       .arg(alert.thresholdValue, 0, 'f', 2);
        } else if (alert.conditionType == Alert::Condition::kAbove) {
            text = QStringLiteral("%1 已涨至 %2%3（提醒阈值 %2%4）")
                       .arg(alert.marketHashName)
                       .arg(symbol)
                       .arg(price, 0, 'f', 2)
                       .arg(alert.thresholdValue, 0, 'f', 2);
        } else {
            text = QStringLiteral("%1 24h 涨跌幅已达阈值 %2%")
                       .arg(alert.marketHashName)
                       .arg(alert.percentValue, 0, 'f', 1);
        }
        m_repo->markTriggered(alert.id, now);
        emit alertTriggered(QStringLiteral("价格提醒"), text);
        qInfo() << "提醒触发:" << text;
    }
}
