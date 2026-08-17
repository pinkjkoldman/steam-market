#pragma once

#include <QString>

// 统一错误码：全局唯一定义处（对应 api-contract.yaml 的 Error schema）。
enum class ErrorCode {
    kOk = 0,
    kInvalidArgument,
    kNotFound,
    kConflict,
    kNetworkError,
    kRateLimited,
    kDataSourceUnavailable,
    kInternal,
};

struct AppError {
    ErrorCode code = ErrorCode::kOk;
    QString message;

    static AppError ok() {
        return {ErrorCode::kOk, QString()};
    }
    static AppError make(ErrorCode c, const QString &m) {
        return {c, m};
    }
    bool isOk() const {
        return code == ErrorCode::kOk;
    }
};
