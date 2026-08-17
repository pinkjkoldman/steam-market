#pragma once

#include <QMetaType>
#include <QString>

enum class AccountErrorCode {
    InvalidTransition, InvalidSteamId, WebViewUnavailable, FreshSessionClearFailed,
    LoginCancelled, LoginNavigationBlocked, SteamUnavailable, SessionRejected,
    PublicInventoryPrivate, InternalError
};

struct AccountError {
    AccountErrorCode code = AccountErrorCode::InternalError;
    QString messageKey;
    bool recoverable = false;
    int retryAfterMs = 0;
};

struct AccountResult {
    bool success = true;
    AccountError error;
    static AccountResult ok() { return {}; }
    static AccountResult failed(AccountErrorCode code, const QString &key,
                                bool recoverable = false, int retryAfterMs = 0) {
        AccountResult result;
        result.success = false;
        result.error = {code, key, recoverable, retryAfterMs};
        return result;
    }
    explicit operator bool() const { return success; }
};

Q_DECLARE_METATYPE(AccountErrorCode)
Q_DECLARE_METATYPE(AccountError)
