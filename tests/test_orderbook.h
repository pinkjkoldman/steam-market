#pragma once

#include <QObject>

class TestOrderbook : public QObject {
    Q_OBJECT

private slots:
    void roundtrip();
};
