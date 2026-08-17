#pragma once

#include <QObject>

class TestAccountEntry : public QObject {
    Q_OBJECT
private slots:
    void accessGateMatrix_data();
    void accessGateMatrix();
    void freshSessionFailureBlocksNavigation();
    void authenticatesAtomicallyAfterFreshSession();
    void cancelDiscardsLatePreparation();
    void logoutClearsBothCookieStores();
    void rejectsMismatchedSteamIdSignal();
    void sessionRejectionExpiresIdentity();
};
