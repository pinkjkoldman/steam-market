#pragma once

#include <QNetworkProxy>
#include <QString>

#include "core/models/AppSettings.h"

// 根据应用设置构建网络代理（供 QNetworkAccessManager 使用）。
// kSystem 由 Qt 自动读取系统代理；kDisabled 强制直连；
// kHttp/kSocks5 使用手动配置的 host:port。
namespace ProxyConfig {

inline QNetworkProxy proxyFromSettings(const AppSettings &s) {
    switch (s.proxyMode) {
        case AppSettings::ProxyMode::kDisabled:
            return QNetworkProxy(QNetworkProxy::NoProxy);
        case AppSettings::ProxyMode::kHttp: {
            QNetworkProxy p(QNetworkProxy::HttpProxy, s.proxyHost.trimmed(),
                            static_cast<quint16>(s.proxyPort));
            return s.proxyHost.trimmed().isEmpty() ? QNetworkProxy(QNetworkProxy::NoProxy) : p;
        }
        case AppSettings::ProxyMode::kSocks5: {
            QNetworkProxy p(QNetworkProxy::Socks5Proxy, s.proxyHost.trimmed(),
                            static_cast<quint16>(s.proxyPort));
            return s.proxyHost.trimmed().isEmpty() ? QNetworkProxy(QNetworkProxy::NoProxy) : p;
        }
        case AppSettings::ProxyMode::kSystem:
        default:
            return QNetworkProxy(QNetworkProxy::DefaultProxy);
    }
}

// 人类可读描述（用于设置页与日志）。
inline QString describe(const AppSettings &s) {
    switch (s.proxyMode) {
        case AppSettings::ProxyMode::kDisabled:
            return QStringLiteral("直连（不使用代理）");
        case AppSettings::ProxyMode::kHttp:
            return QStringLiteral("HTTP %1:%2").arg(s.proxyHost.trimmed()).arg(s.proxyPort);
        case AppSettings::ProxyMode::kSocks5:
            return QStringLiteral("SOCKS5 %1:%2").arg(s.proxyHost.trimmed()).arg(s.proxyPort);
        case AppSettings::ProxyMode::kSystem:
        default:
            return QStringLiteral("跟随系统");
    }
}

}  // namespace ProxyConfig
