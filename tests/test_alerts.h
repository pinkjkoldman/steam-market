#pragma once

#include <QObject>

class TestAlerts : public QObject {
    Q_OBJECT

private slots:
    void triggersBelowAndAbove();
    void cooldownPreventsRepeat();
    void percentTrigger();
};
