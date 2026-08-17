#pragma once

#include <QString>

#include "core/models/AppSettings.h"

// 全局涨跌配色：由设置页写入，行情涨跌着色统一从这里取值，
// 避免「涨跌配色」设置只保存不生效。
namespace ThemeProvider {

inline AppSettings::ColorScheme &storage() {
    static AppSettings::ColorScheme scheme = AppSettings::ColorScheme::kRedUpGreenDown;
    return scheme;
}

inline void setScheme(AppSettings::ColorScheme scheme) {
    storage() = scheme;
}

inline AppSettings::ColorScheme scheme() {
    return storage();
}

inline QString upColor() {
    return scheme() == AppSettings::ColorScheme::kGreenUpRedDown
               ? QStringLiteral("#46A758")
               : QStringLiteral("#E5484D");
}

inline QString downColor() {
    return scheme() == AppSettings::ColorScheme::kGreenUpRedDown
               ? QStringLiteral("#E5484D")
               : QStringLiteral("#46A758");
}

}  // namespace ThemeProvider
