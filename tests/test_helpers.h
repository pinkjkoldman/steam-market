#pragma once

#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QString>

#include "data/DatabaseManager.h"

// 测试夹具：每个用例独立的临时数据库。
struct TestDb {
    QTemporaryDir dir;
    DatabaseManager db;
    QSqlDatabase handle;
    bool opened = false;

    TestDb() : db(dir.filePath(QStringLiteral("test.db"))) {
        opened = db.open();
        if (opened) {
            handle = db.database();
        }
    }
    ~TestDb() {
        handle = QSqlDatabase();
        db.close();
    }
};
