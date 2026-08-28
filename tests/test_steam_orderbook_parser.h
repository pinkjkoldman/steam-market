#pragma once

#include <QObject>

class TestSteamOrderbookParser : public QObject {
    Q_OBJECT

private slots:
    void parsesQueryActionPayload();
    void rejectsLegacyAndMalformedShapes();
    void clientUsesOfficialQueryActionProtocol();
};
