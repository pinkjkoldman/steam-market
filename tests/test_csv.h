#pragma once

#include <QObject>

class TestCsv : public QObject {
    Q_OBJECT

private slots:
    void importRows();
    void invalidFile();
};
