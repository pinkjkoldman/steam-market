#include "core/services/SteamSessionService.h"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QRegularExpression>
#include <QUrl>

#include "network/IWebSessionHost.h"

namespace {
QString normalizedSteamId(const QString &value) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9]{17}$"));
    const QString candidate = value.trimmed();
    return pattern.match(candidate).hasMatch() ? candidate : QString();
}

QString steamIdFromCookies(const QList<QNetworkCookie> &cookies) {
    static const QRegularExpression loginValue(QStringLiteral("^([0-9]{17})(?:\\|\\||$)"));
    for (const QNetworkCookie &cookie : cookies) {
        if (cookie.name() != QByteArray("steamLoginSecure")) continue;
        const QString decoded = QUrl::fromPercentEncoding(cookie.value()).trimmed();
        const QRegularExpressionMatch match = loginValue.match(decoded);
        if (match.hasMatch()) return match.captured(1);
    }
    return {};
}
}  // namespace

SteamSessionService::SteamSessionService(IWebSessionHost *host,
                                         QNetworkAccessManager *network, QObject *parent)
    : QObject(parent), m_host(host), m_network(network) {
    qRegisterMetaType<IdentitySnapshot>();
    qRegisterMetaType<AccountError>();
    connect(host, &IWebSessionHost::sessionCookiesReady, this,
            &SteamSessionService::adoptCookies);
    connect(host, &IWebSessionHost::surfaceClosedByUser, this, [this]() {
        if (m_identity.state == IdentityState::Authenticating) cancelOfficialLogin();
    });
    connect(host, &IWebSessionHost::hostError, this, [this](const QString &) {
        if (m_identity.state != IdentityState::Authenticating || !m_loginSurfaceOpen) return;
        ++m_generation;
        m_host->dismissSurface();
        publish(m_loginOrigin);
        m_loginSurfaceOpen = false;
        emit loginSurfaceClosed();
        reject(AccountErrorCode::LoginNavigationBlocked,
               QStringLiteral("account.login.navigation_blocked"), true);
    });
}

void SteamSessionService::showLogin() { beginOfficialLogin(); }

AccountResult SteamSessionService::beginOfficialLogin() {
    const IdentityState state = m_identity.state;
    if (state != IdentityState::ChoiceRequired && state != IdentityState::Guest
        && state != IdentityState::PublicInventory && state != IdentityState::Expired) {
        return reject(AccountErrorCode::InvalidTransition,
                      QStringLiteral("account.error.invalid_transition"));
    }
    m_loginOrigin = m_identity;
    IdentitySnapshot authenticating = m_identity;
    authenticating.state = IdentityState::Authenticating;
    authenticating.statusMessage = QStringLiteral("account.status.authenticating");
    authenticating.busy = true;
    publish(authenticating);
    const quint64 generation = ++m_generation;
    m_host->prepareFreshLoginSession([this, generation](FreshSessionResult result) {
        if (generation != m_generation || m_identity.state != IdentityState::Authenticating) return;
        if (result != FreshSessionResult::Ready) {
            m_host->dismissSurface();
            publish(m_loginOrigin);
            const bool unavailable = result == FreshSessionResult::Unavailable;
            reject(unavailable ? AccountErrorCode::WebViewUnavailable
                               : AccountErrorCode::FreshSessionClearFailed,
                   unavailable ? QStringLiteral("account.webview.unavailable")
                               : QStringLiteral("account.webview.fresh_session_clear_failed"), true);
            return;
        }
        clearNetworkCookies();
        m_loginSurfaceOpen = true;
        emit loginSurfaceRequested();
        m_host->showLogin();
    });
    return AccountResult::ok();
}

AccountResult SteamSessionService::cancelOfficialLogin() {
    if (m_identity.state != IdentityState::Authenticating) {
        return reject(AccountErrorCode::InvalidTransition,
                      QStringLiteral("account.error.invalid_transition"));
    }
    ++m_generation;
    m_host->dismissSurface();
    publish(m_loginOrigin);
    m_loginSurfaceOpen = false;
    emit loginSurfaceClosed();
    return AccountResult::ok();
}

AccountResult SteamSessionService::chooseGuest() {
    if (m_identity.state != IdentityState::ChoiceRequired) {
        return reject(AccountErrorCode::InvalidTransition,
                      QStringLiteral("account.error.invalid_transition"));
    }
    publish({IdentityState::Guest, {}, {}, QStringLiteral("account.status.guest"), false});
    return AccountResult::ok();
}

AccountResult SteamSessionService::usePublicInventory(const QString &steamId) {
    if (m_identity.state != IdentityState::Guest && m_identity.state != IdentityState::Expired) {
        return reject(AccountErrorCode::InvalidTransition,
                      QStringLiteral("account.error.invalid_transition"));
    }
    const QString normalized = normalizedSteamId(steamId);
    if (normalized.isEmpty()) {
        return reject(AccountErrorCode::InvalidSteamId,
                      QStringLiteral("account.error.invalid_steam_id"), true);
    }
    publish({IdentityState::PublicInventory, normalized, QStringLiteral("公开库存"),
             QStringLiteral("account.status.public_inventory"), false});
    return AccountResult::ok();
}

AccountResult SteamSessionService::clearPublicIdentity() {
    if (m_identity.state != IdentityState::PublicInventory) {
        return reject(AccountErrorCode::InvalidTransition,
                      QStringLiteral("account.error.invalid_transition"));
    }
    publish({IdentityState::Guest, {}, {}, QStringLiteral("account.status.guest"), false});
    return AccountResult::ok();
}

AccountResult SteamSessionService::markSessionRejected() {
    if (m_identity.state != IdentityState::Authenticated) {
        return reject(AccountErrorCode::InvalidTransition,
                      QStringLiteral("account.error.invalid_transition"));
    }
    publish({IdentityState::Expired, m_identity.steamId64, m_identity.displayName,
             QStringLiteral("account.status.expired"), false});
    return AccountResult::ok();
}

void SteamSessionService::openOfficialUrl(const QUrl &url) { m_host->openOfficialUrl(url); }

AccountResult SteamSessionService::logout() {
    ++m_generation;
    m_host->dismissSurface();
    m_host->clearSession();
    clearNetworkCookies();
    publish({IdentityState::Guest, {}, {}, QStringLiteral("account.status.guest"), false});
    m_loginSurfaceOpen = false;
    emit loginSurfaceClosed();
    return AccountResult::ok();
}

void SteamSessionService::clearNetworkCookies() {
    if (!m_network || !m_network->cookieJar()) return;
    QNetworkCookieJar *jar = m_network->cookieJar();
    const QList<QUrl> urls = {QUrl(QStringLiteral("https://steamcommunity.com")),
                              QUrl(QStringLiteral("https://login.steampowered.com"))};
    for (const QUrl &url : urls) {
        const QList<QNetworkCookie> cookies = jar->cookiesForUrl(url);
        for (const QNetworkCookie &cookie : cookies) jar->deleteCookie(cookie);
    }
}

void SteamSessionService::adoptCookies(const QString &steamId,
                                       const QList<QNetworkCookie> &cookies) {
    if (m_identity.state != IdentityState::Authenticating || !m_loginSurfaceOpen) return;
    const QString cookieSteamId = steamIdFromCookies(cookies);
    const QString signalledSteamId = normalizedSteamId(steamId);
    const QString resolvedSteamId = signalledSteamId.isEmpty() ? cookieSteamId : signalledSteamId;
    if (cookieSteamId.isEmpty() || resolvedSteamId.isEmpty()
        || (!signalledSteamId.isEmpty() && signalledSteamId != cookieSteamId)) return;
    if (m_network && m_network->cookieJar()) {
        for (const QNetworkCookie &cookie : cookies) {
            m_network->cookieJar()->setCookiesFromUrl(
                {cookie}, QUrl(QStringLiteral("https://steamcommunity.com")));
        }
    }
    ++m_generation;
    m_host->dismissSurface();
    publish({IdentityState::Authenticated, resolvedSteamId,
             QStringLiteral("Steam %1").arg(resolvedSteamId.right(6)),
             QStringLiteral("account.status.authenticated"), false});
    m_loginSurfaceOpen = false;
    emit loginSurfaceClosed();
}

void SteamSessionService::publish(const IdentitySnapshot &snapshot) {
    if (snapshot == m_identity) return;
    m_identity = snapshot;
    emit identityChanged(m_identity);
    emit sessionChanged(m_identity.state == IdentityState::Authenticated,
                        m_identity.steamId64, m_identity.displayName);
}

AccountResult SteamSessionService::reject(AccountErrorCode code, const QString &messageKey,
                                          bool recoverable) {
    const AccountResult result = AccountResult::failed(code, messageKey, recoverable);
    emit operationFailed(result.error);
    emit sessionError(messageKey);
    return result;
}
