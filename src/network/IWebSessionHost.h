#pragma once

#include <QNetworkCookie>
#include <QObject>
#include <QUrl>

#include <functional>

enum class FreshSessionResult { Ready, Unavailable, ClearFailed };

class IWebSessionHost : public QObject {
    Q_OBJECT

public:
    explicit IWebSessionHost(QObject *parent = nullptr) : QObject(parent) {}
    ~IWebSessionHost() override = default;

    virtual void showLogin() = 0;
    virtual void openOfficialUrl(const QUrl &url) = 0;
    virtual void clearSession() = 0;
    virtual void prepareFreshLoginSession(
        std::function<void(FreshSessionResult)> completion) {
        completion(FreshSessionResult::Unavailable);
    }

signals:
    void sessionCookiesReady(const QString &steamId, const QList<QNetworkCookie> &cookies);
    void hostError(const QString &message);
};
