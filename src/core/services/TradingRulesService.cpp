#include "core/services/TradingRulesService.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtDebug>

#include "core/services/SettingsService.h"

namespace {
double round2(double v) {
    return qRound(v * 100.0) / 100.0;
}
}  // namespace

TradingRulesService::TradingRulesService(SettingsService *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {
    QFile f(QStringLiteral(":/rules/rules/rules.json"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "读取交易规则资源失败";
        return;
    }
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        TradingRule r;
        r.id = o.value(QStringLiteral("id")).toString();
        r.category = o.value(QStringLiteral("category")).toString();
        r.title = o.value(QStringLiteral("title")).toString();
        r.content = o.value(QStringLiteral("content")).toString();
        r.source = o.value(QStringLiteral("source")).toString();
        r.updatedAt = QDate::fromString(o.value(QStringLiteral("updatedAt")).toString(),
                                        Qt::ISODate);
        if (!r.id.isEmpty()) {
            m_rules.append(r);
        }
    }
}

QVector<TradingRule> TradingRulesService::rules() const {
    return m_rules;
}

QStringList TradingRulesService::categories() const {
    QStringList out;
    for (const TradingRule &r : m_rules) {
        if (!out.contains(r.category)) {
            out << r.category;
        }
    }
    return out;
}

QVector<TradingRule> TradingRulesService::rulesByCategory(const QString &category) const {
    QVector<TradingRule> out;
    for (const TradingRule &r : m_rules) {
        if (r.category == category) {
            out.append(r);
        }
    }
    return out;
}

FeeEstimate TradingRulesService::estimateFee(const QString &direction, double price) const {
    FeeEstimate out;
    out.direction = direction;
    out.inputPrice = price;
    out.currency = m_settings ? m_settings->settings().currency : QStringLiteral("CNY");
    const double steamRate = m_settings ? m_settings->settings().feeSteamRate : 0.05;
    const double gameRate = m_settings ? m_settings->settings().feeGameRate : 0.10;
    const double totalRate = steamRate + gameRate;
    if (direction == QLatin1String("sell")) {
        // 输入 = 卖家想得到的金额；买家支付 = 到手价 × (1 + 总费率)
        out.sellerReceives = round2(price);
        out.steamFee = round2(price * steamRate);
        out.gameFee = round2(price * gameRate);
        out.totalFee = round2(out.steamFee + out.gameFee);
        out.buyerPays = round2(price + out.totalFee);
    } else {
        // 输入 = 买家支付；卖家到手 = 买家支付 / (1 + 总费率)
        out.buyerPays = round2(price);
        out.sellerReceives = round2(price / (1.0 + totalRate));
        out.totalFee = round2(price - out.sellerReceives);
        out.steamFee = round2(out.totalFee * steamRate / totalRate);
        out.gameFee = round2(out.totalFee - out.steamFee);
    }
    return out;
}
