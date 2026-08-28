#include "core/services/SettingsService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtDebug>

#include "data/repositories/SettingsRepository.h"

namespace {
QString jsonOf(const AppSettings &s) {
    QJsonObject o;
    o.insert(QStringLiteral("currency"), s.currency);
    o.insert(QStringLiteral("refreshIntervalMinutes"), s.refreshIntervalMinutes);
    o.insert(QStringLiteral("colorScheme"),
             s.colorScheme == AppSettings::ColorScheme::kGreenUpRedDown ? 1 : 0);
    o.insert(QStringLiteral("notificationsEnabled"), s.notificationsEnabled);
    o.insert(QStringLiteral("trayOnClose"), s.trayOnClose);
    o.insert(QStringLiteral("requestIntervalMs"), s.requestIntervalMs);
    o.insert(QStringLiteral("feeSteamRate"), s.feeSteamRate);
    o.insert(QStringLiteral("feeGameRate"), s.feeGameRate);
    o.insert(QStringLiteral("gameAppid"), s.gameAppid);
    o.insert(QStringLiteral("startupIdentityMode"),
             s.startupIdentityMode == AppSettings::StartupIdentityMode::kGuest
                 ? QStringLiteral("guest") : QStringLiteral("ask"));
    o.insert(QStringLiteral("proxyMode"), static_cast<int>(s.proxyMode));
    o.insert(QStringLiteral("proxyHost"), s.proxyHost);
    o.insert(QStringLiteral("proxyPort"), s.proxyPort);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

AppSettings settingsOf(const QString &json) {
    AppSettings s;
    const QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
    if (o.isEmpty()) return s;
    s.currency = o.value(QStringLiteral("currency")).toString(QStringLiteral("CNY"));
    s.refreshIntervalMinutes =
        qBound(1, o.value(QStringLiteral("refreshIntervalMinutes")).toInt(10), 1440);
    s.colorScheme = o.value(QStringLiteral("colorScheme")).toInt(0) == 1
                        ? AppSettings::ColorScheme::kGreenUpRedDown
                        : AppSettings::ColorScheme::kRedUpGreenDown;
    s.notificationsEnabled = o.value(QStringLiteral("notificationsEnabled")).toBool(true);
    s.trayOnClose = o.value(QStringLiteral("trayOnClose")).toBool(true);
    s.requestIntervalMs = qBound(500, o.value(QStringLiteral("requestIntervalMs")).toInt(1500), 10000);
    s.feeSteamRate = qBound(0.0, o.value(QStringLiteral("feeSteamRate")).toDouble(0.05), 0.5);
    s.feeGameRate = qBound(0.0, o.value(QStringLiteral("feeGameRate")).toDouble(0.10), 0.5);
    const int appid = o.value(QStringLiteral("gameAppid")).toInt(730);
    s.gameAppid = (appid == 570 || appid == 440) ? appid : 730;
    const QString startupMode = o.value(QStringLiteral("startupIdentityMode"))
                                    .toString(QStringLiteral("ask"));
    s.startupIdentityMode = startupMode == QLatin1String("guest")
                                ? AppSettings::StartupIdentityMode::kGuest
                                : AppSettings::StartupIdentityMode::kAskEveryTime;
    const int proxyMode = qBound(0, o.value(QStringLiteral("proxyMode")).toInt(0), 3);
    s.proxyMode = static_cast<AppSettings::ProxyMode>(proxyMode);
    s.proxyHost = o.value(QStringLiteral("proxyHost")).toString();
    s.proxyPort = qBound(1, o.value(QStringLiteral("proxyPort")).toInt(1080), 65535);
    return s;
}
}  // namespace

SettingsService::SettingsService(SettingsRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo) {
    m_settings = load();
}

AppSettings SettingsService::load() const {
    if (!m_repo) return AppSettings();
    const QString raw = m_repo->value(QStringLiteral("app_settings"));
    if (raw.isEmpty()) return AppSettings();
    return settingsOf(raw);
}

bool SettingsService::save(const AppSettings &next) {
    if (next.refreshIntervalMinutes < 1 || next.refreshIntervalMinutes > 1440) {
        qWarning() << "刷新间隔非法:" << next.refreshIntervalMinutes;
        return false;
    }
    if (!m_repo->setValue(QStringLiteral("app_settings"), jsonOf(next))) {
        return false;
    }
    m_settings = next;
    emit settingsChanged(m_settings);
    return true;
}
