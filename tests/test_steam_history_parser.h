#pragma once

#include <QObject>

class TestSteamHistoryParser : public QObject {
    Q_OBJECT

private slots:
    void parsesRealShapeAndStringVolumes();
    void sortsAndDeduplicatesTimestamps();
    void distinguishesEmptyFromInvalid();
    void rejectsSchemaDrift();
};

