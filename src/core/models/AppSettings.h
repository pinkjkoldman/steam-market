#pragma once

#include <QString>

// 应用设置（对应 api-contract.yaml Settings）。
struct AppSettings {
    enum class ColorScheme { kRedUpGreenDown, kGreenUpRedDown };
    enum class StartupIdentityMode { kAskEveryTime, kGuest };
    // 代理模式：跟随系统 / 直连（禁用）/ HTTP / SOCKS5
    enum class ProxyMode { kSystem, kDisabled, kHttp, kSocks5 };

    QString currency = QStringLiteral("CNY");
    int refreshIntervalMinutes = 10;
    ColorScheme colorScheme = ColorScheme::kRedUpGreenDown;
    bool notificationsEnabled = true;
    bool trayOnClose = true;
    int requestIntervalMs = 1500;
    // v2：费用计算费率（默认 CS2：Steam 5% + 游戏 10%，按卖家到手价计）
    double feeSteamRate = 0.05;
    double feeGameRate = 0.10;
    // v3：默认游戏（CS2=730 / DOTA2=570 / TF2=440 预留）
    int gameAppid = 730;
    StartupIdentityMode startupIdentityMode = StartupIdentityMode::kAskEveryTime;
    // 网络代理（默认跟随系统；直连或手动 HTTP/SOCKS5 可选）
    ProxyMode proxyMode = ProxyMode::kSystem;
    QString proxyHost;
    int proxyPort = 1080;
};
