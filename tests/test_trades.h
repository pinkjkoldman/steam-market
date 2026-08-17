#pragma once

#include <QObject>

class TestTrades : public QObject {
    Q_OBJECT

private slots:
    void buySellLifecycle();
    void oversellRejected();
};
