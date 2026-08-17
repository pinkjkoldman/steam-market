#pragma once

#include <QObject>

class TestFee : public QObject {
    Q_OBJECT

private slots:
    void sellDirection();
    void buyDirection();
};
