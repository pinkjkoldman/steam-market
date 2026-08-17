#pragma once

#include <QNetworkCookie>
#include <QObject>
#include <QUrl>

#include "core/models/IdentityAccount.h"
#include "core/models/IdentitySnapshot.h"

class IWebSessionHost;
class QNetworkAccessManager;

class SteamSessionService : public QObject {
    Q_OBJECT

public:
    SteamSessionService(IWebSessionHost *host, QNetworkAccessManager *network,
                        QObject *parent = nullptr);

    IdentitySnapshot snapshot() const { return m_identity; }
    bool hasSession() const { return !m_identity.steamId64.isEmpty(); }
    bool isAuthenticated() const { return m_identity.state == IdentityState::Authenticated; }
    QString steamId() const { return m_identity.steamId64; }
    QString displayName() const { return m_identity.displayName; }

    void showLogin();
    AccountResult beginOfficialLogin();
    AccountResult cancelOfficialLogin();
    AccountResult chooseGuest();
    AccountResult usePublicInventory(const QString &steamId);
    AccountResult clearPublicIdentity();
    AccountResult markSessionRejected();
    void openOfficialUrl(const QUrl &url);
    AccountResult logout();

signals:
    void sessionChanged(bool authenticated, const QString &steamId,
                        const QString &displayName);
    void sessionError(const QString &message);
    void identityChanged(const IdentitySnapshot &snapshot);
    void loginSurfaceRequested();
    void loginSurfaceClosed();
    void operationFailed(const AccountError &error);

private:
    void adoptCookies(const QString &steamId, const QList<QNetworkCookie> &cookies);
    void publish(const IdentitySnapshot &snapshot);
    AccountResult reject(AccountErrorCode code, const QString &messageKey,
                         bool recoverable = false);
    void clearNetworkCookies();

    IWebSessionHost *m_host = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    IdentitySnapshot m_identity;
    IdentitySnapshot m_loginOrigin;
    quint64 m_generation = 0;
    bool m_loginSurfaceOpen = false;
};