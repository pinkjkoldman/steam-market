#pragma once

#include <QObject>

class TestPortfolio : public QObject {
    Q_OBJECT

private slots:
    void valuationMath();
    void missingPriceExcluded();
};
