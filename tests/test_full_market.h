#pragma once

#include <QObject>

class TestFullMarket : public QObject {
    Q_OBJECT

private slots:
    void parsesFixedSteamFixture();
    void rejectsChangedSchemaAtomically();
    void validatesQueryAndUrl();
    void boundsRetryAfterDelay();
    void cacheKeySeparatesDimensions();
    void repositoryRoundtripAndSnapshotDeduplication();
    void migrationUpgradesExistingSingleKeyItemsTable();
    void filtersCurrentPageAndBuildsVisualSummary();
};
