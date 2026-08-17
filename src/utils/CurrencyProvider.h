#pragma once

#include <QString>

// 全局生效币种：所有价格缓存查询与展示统一使用该代码，由设置页写入。
// 默认 CNY，保证未初始化（如单元测试）时行为与历史一致。
namespace CurrencyProvider {

inline QString &storage() {
    static QString code = QStringLiteral("CNY");
    return code;
}

inline QString code() {
    return storage();
}

inline void setCode(const QString &code) {
    storage() = code.trimmed().isEmpty() ? QStringLiteral("CNY") : code.trimmed();
}

}  // namespace CurrencyProvider
