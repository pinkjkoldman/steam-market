#pragma once

#include <QObject>

class TestDatabase : public QObject {
    Q_OBJECT

private slots:
    void migrationApplied();
    void itemUpsert();
    void priceSnapshotAndHistory();
    void watchlistCrud();
    void alertCrud();
    void portfolioCrud();
    void settingsRoundtrip();
};
