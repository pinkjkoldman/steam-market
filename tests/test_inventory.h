#pragma once

#include <QObject>

class TestInventory : public QObject {
    Q_OBJECT

private slots:
    void parserJoinsAssetsAndDescriptions();
    void parserRejectsInvalidPayload();
    void repositoryCompletesAtomicSnapshot();
    void fixedPriceDraftUsesMinorUnits();
    void handoffSplitsLargeSelections();
    void sessionDerivesSteamIdFromLoginCookie();
};
