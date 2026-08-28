#include "test_account_entry.h"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>

#include <utility>
#include <QtTest>

#include "core/services/AccessGate.h"
#include "core/services/SteamSessionService.h"
#include "network/IWebSessionHost.h"

namespace {
class FakeSessionHost final : public IWebSessionHost {
public:
    void showLogin() override { ++showLoginCount; }
    void openOfficialUrl(const QUrl &) override {}
    void clearSession() override { ++clearCount; }
    void prepareFreshLoginSession(std::function<void(FreshSessionResult)> callback) override {
        ++prepareCount;
        completion = std::move(callback);
    }
    void complete(FreshSessionResult result) {
        auto callback = std::move(completion);
        if (callback) callback(result);
    }
    void publishLogin(const QString &steamId = {}) {
        QNetworkCookie cookie(QByteArray("steamLoginSecure"),
                              QByteArray("76561198000000000%7C%7Cfixture"));
        cookie.setDomain(QStringLiteral(".steamcommunity.com"));
        cookie.setPath(QStringLiteral("/"));
        emit sessionCookiesReady(steamId, {cookie});
    }
    void closeSurface() { emit surfaceClosedByUser(); }
    int showLoginCount = 0;
    int clearCount = 0;
    int prepareCount = 0;
    std::function<void(FreshSessionResult)> completion;
};
}

void TestAccountEntry::accessGateMatrix_data() {
    QTest::addColumn<IdentityState>("state");
    QTest::addColumn<Capability>("capability");
    QTest::addColumn<AccessDecision>("decision");
    const QList<IdentityState> states = {IdentityState::ChoiceRequired, IdentityState::Guest,
        IdentityState::PublicInventory, IdentityState::Authenticating,
        IdentityState::Authenticated, IdentityState::Expired};
    const QList<Capability> capabilities = {Capability::PublicMarket, Capability::LocalWatchlist,
        Capability::EnterPublicSteamId, Capability::PublicInventory, Capability::PrivateInventory,
        Capability::AccountAction, Capability::OpenOfficialCommunity};
    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
        for (int capabilityIndex = 0; capabilityIndex < capabilities.size(); ++capabilityIndex) {
            const auto state = states.at(stateIndex);
            const auto capability = capabilities.at(capabilityIndex);
            AccessDecision expected = AccessDecision::RequireLogin;
            if (state == IdentityState::ChoiceRequired) expected = AccessDecision::RequireChoice;
            else if (capability == Capability::PublicMarket || capability == Capability::LocalWatchlist)
                expected = AccessDecision::Allow;
            else if (state == IdentityState::Authenticating) expected = AccessDecision::Busy;
            else if (capability == Capability::EnterPublicSteamId
                     || capability == Capability::OpenOfficialCommunity) expected = AccessDecision::Allow;
            else if (capability == Capability::PublicInventory)
                expected = (state == IdentityState::PublicInventory || state == IdentityState::Authenticated)
                               ? AccessDecision::Allow : AccessDecision::RequirePublicId;
            else if (state == IdentityState::Authenticated) expected = AccessDecision::Allow;
            else if (state == IdentityState::Expired) expected = AccessDecision::Reauthenticate;
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(stateIndex).arg(capabilityIndex)))
                << state << capability << expected;
        }
    }
}

void TestAccountEntry::accessGateMatrix() {
    QFETCH(IdentityState, state);
    QFETCH(Capability, capability);
    QFETCH(AccessDecision, decision);
    IdentitySnapshot identity;
    identity.state = state;
    QCOMPARE(AccessGate::evaluate(capability, identity), decision);
}

void TestAccountEntry::freshSessionFailureBlocksNavigation() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QVERIFY(service.beginOfficialLogin());
    QCOMPARE(service.snapshot().state, IdentityState::Authenticating);
    host.complete(FreshSessionResult::ClearFailed);
    QCOMPARE(host.showLoginCount, 0);
    QCOMPARE(service.snapshot().state, IdentityState::ChoiceRequired);
}

void TestAccountEntry::authenticatesAtomicallyAfterFreshSession() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QSignalSpy changed(&service, &SteamSessionService::identityChanged);
    QVERIFY(service.beginOfficialLogin());
    host.complete(FreshSessionResult::Ready);
    QCOMPARE(host.showLoginCount, 1);
    host.publishLogin();
    QCOMPARE(service.snapshot().state, IdentityState::Authenticated);
    QCOMPARE(service.snapshot().steamId64, QStringLiteral("76561198000000000"));
    QVERIFY(!service.snapshot().busy);
    const IdentitySnapshot published = changed.last().at(0).value<IdentitySnapshot>();
    QCOMPARE(published.state, IdentityState::Authenticated);
    QCOMPARE(published.steamId64, QStringLiteral("76561198000000000"));
}

void TestAccountEntry::cancelDiscardsLatePreparation() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QVERIFY(service.beginOfficialLogin());
    QVERIFY(service.cancelOfficialLogin());
    host.complete(FreshSessionResult::Ready);
    QCOMPARE(host.showLoginCount, 0);
    QCOMPARE(service.snapshot().state, IdentityState::ChoiceRequired);
}

void TestAccountEntry::closingLoginSurfaceRestoresOrigin() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QSignalSpy closed(&service, &SteamSessionService::loginSurfaceClosed);
    QVERIFY(service.beginOfficialLogin());
    host.complete(FreshSessionResult::Ready);
    QCOMPARE(service.snapshot().state, IdentityState::Authenticating);
    host.closeSurface();
    QCOMPARE(service.snapshot().state, IdentityState::ChoiceRequired);
    QCOMPARE(closed.count(), 1);
}

void TestAccountEntry::logoutClearsBothCookieStores() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QVERIFY(service.beginOfficialLogin());
    host.complete(FreshSessionResult::Ready);
    host.publishLogin();
    QCOMPARE(service.snapshot().state, IdentityState::Authenticated);
    QVERIFY(!network.cookieJar()
                 ->cookiesForUrl(QUrl(QStringLiteral("https://steamcommunity.com")))
                 .isEmpty());

    QVERIFY(service.logout());
    QCOMPARE(host.clearCount, 1);
    QCOMPARE(service.snapshot().state, IdentityState::Guest);
    QVERIFY(network.cookieJar()
                ->cookiesForUrl(QUrl(QStringLiteral("https://steamcommunity.com")))
                .isEmpty());
}

void TestAccountEntry::rejectsMismatchedSteamIdSignal() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QVERIFY(service.beginOfficialLogin());
    host.complete(FreshSessionResult::Ready);
    host.publishLogin(QStringLiteral("76561198000000001"));
    QCOMPARE(service.snapshot().state, IdentityState::Authenticating);
    QVERIFY(service.snapshot().steamId64.isEmpty());
}

void TestAccountEntry::sessionRejectionExpiresIdentity() {
    FakeSessionHost host;
    QNetworkAccessManager network;
    SteamSessionService service(&host, &network);
    QVERIFY(service.beginOfficialLogin());
    host.complete(FreshSessionResult::Ready);
    host.publishLogin();
    QVERIFY(service.markSessionRejected());
    QCOMPARE(service.snapshot().state, IdentityState::Expired);
    QCOMPARE(AccessGate::evaluate(Capability::PrivateInventory, service.snapshot()),
             AccessDecision::Reauthenticate);
}
