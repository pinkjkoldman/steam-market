#pragma once

#include <QString>

// Steam 市场接口使用数字币种 ID；应用内部用 ISO 代码（CNY/USD/EUR/RUB）。
namespace Currency {

inline QString defaultCode() {
    return QStringLiteral("CNY");
}

inline int steamId(const QString &code) {
    if (code == QLatin1String("USD")) return 1;
    if (code == QLatin1String("EUR")) return 3;
    if (code == QLatin1String("RUB")) return 5;
    return 23;  // CNY
}

inline QString codeFromSteamId(int id) {
    if (id == 1) return QStringLiteral("USD");
    if (id == 3) return QStringLiteral("EUR");
    if (id == 5) return QStringLiteral("RUB");
    if (id == 23) return QStringLiteral("CNY");
    return QString();
}

inline QString displaySymbol(const QString &code) {
    if (code == QLatin1String("USD")) return QStringLiteral("$");
    if (code == QLatin1String("EUR")) return QStringLiteral("€");
    if (code == QLatin1String("RUB")) return QStringLiteral("₽");
    return QStringLiteral("¥");
}

}  // namespace Currency
