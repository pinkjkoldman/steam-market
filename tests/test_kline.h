#pragma once

#include <QObject>

class TestKline : public QObject {
    Q_OBJECT

private slots:
    void dailyAggregation();
    void movingAverage();
    void sortsPointsAndPreservesVolumeMeaning();
};
