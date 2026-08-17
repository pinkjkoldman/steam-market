#pragma once

#include <functional>
#include <optional>

#include "core/errors.h"
#include "core/models/PlatformPrice.h"

// 比价数据源抽象：接入 BUFF/C5 等平台时实现本接口并注册。
class IPriceSource {
public:
    virtual ~IPriceSource() = default;
    virtual QString platform() const = 0;
    virtual void fetchPrice(const QString &marketHashName,
                            std::function<void(std::optional<PlatformPrice>, const AppError &)> cb) = 0;
};
