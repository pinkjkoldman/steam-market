#include "data/DatabaseManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPair>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <QtDebug>

DatabaseManager::DatabaseManager(const QString &dbPath)
    : m_dbPath(dbPath),
      m_connectionName(QStringLiteral("smt_%1").arg(quintptr(this))) {}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::open() {
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        qCritical() << "QSQLITE 驱动不可用，可用驱动:" << QSqlDatabase::drivers().join(QLatin1Char(','));
        return false;
    }
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qCritical() << "打开数据库失败:" << db.lastError().text();
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    return migrate();
}

void DatabaseManager::close() {
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DatabaseManager::isOpen() const {
    return QSqlDatabase::contains(m_connectionName)
           && QSqlDatabase::database(m_connectionName).isOpen();
}

QSqlDatabase DatabaseManager::database() const {
    return QSqlDatabase::database(m_connectionName);
}

bool DatabaseManager::migrate() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery init(db);
    if (!init.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            " version INTEGER PRIMARY KEY, applied_at DATETIME NOT NULL DEFAULT (datetime('now')) )"))) {
        qCritical() << "初始化 schema_migrations 失败:" << init.lastError().text();
        return false;
    }

    // 迁移脚本以资源方式内嵌，随应用发布，避免路径依赖。
    // 版本号 → 迁移脚本（随 QRC 内嵌）
    const QList<QPair<QString, QString>> migrations = {
        {QStringLiteral("1"), QStringLiteral("0001_init.sql")},
        {QStringLiteral("2"), QStringLiteral("0002_v2.sql")},
        {QStringLiteral("3"), QStringLiteral("0003_v3.sql")},
        {QStringLiteral("4"), QStringLiteral("0004_v3_autotrade.sql")},
        {QStringLiteral("5"), QStringLiteral("0005_v4_inventory_assistant.sql")},
        {QStringLiteral("6"), QStringLiteral("0006_cr004_market_catalog.sql")},
    };
    for (const auto &migration : migrations) {
        const QString &v = migration.first;
        QSqlQuery check(db);
        check.prepare(QStringLiteral("SELECT COUNT(*) FROM schema_migrations WHERE version = ?"));
        check.addBindValue(v.toInt());
        if (!check.exec() || !check.next() || check.value(0).toInt() > 0) {
            continue;
        }
        const QString sqlPath = QStringLiteral(":/migrations/migrations/%1").arg(migration.second);
        QFile f(sqlPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCritical() << "读取迁移脚本失败:" << sqlPath;
            return false;
        }
        const QString sql = QString::fromUtf8(f.readAll());
        db.transaction();
        QSqlQuery query(db);
        const QStringList statements = sql.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        bool ok = true;
        for (const QString &stmt : statements) {
            if (stmt.trimmed().isEmpty()) continue;
            if (!query.exec(stmt)) {
                qCritical() << "迁移失败(version" << v << "):" << query.lastError().text();
                ok = false;
                break;
            }
        }
        if (ok) {
            QSqlQuery mark(db);
            mark.prepare(QStringLiteral("INSERT INTO schema_migrations (version) VALUES (?)"));
            mark.addBindValue(v.toInt());
            ok = mark.exec();
            if (!ok) {
                qCritical() << "记录迁移版本失败(version" << v << "):" << mark.lastError().text();
            }
        }
        if (ok) {
            db.commit();
        } else {
            db.rollback();
        }
        if (!ok) return false;
    }
    return true;
}

bool DatabaseManager::backup() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen()) return false;
    QSqlQuery(db).exec(QStringLiteral("PRAGMA wal_checkpoint(FULL)"));
    const QString backupPath = m_dbPath + QStringLiteral(".bak");
    QFile::remove(backupPath);
    if (!QFile::copy(m_dbPath, backupPath)) {
        qCritical() << "备份失败:" << m_dbPath;
        return false;
    }
    qInfo() << "数据库已备份到" << backupPath;
    return true;
}

void DatabaseManager::runRetentionPolicy() {
    if (!isOpen()) return;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    // 快照保留 90 天，历史保留 5 年。
    q.prepare(QStringLiteral("DELETE FROM price_snapshots WHERE created_at < datetime('now','-90 days')"));
    q.exec();
    q.prepare(QStringLiteral("DELETE FROM price_history WHERE recorded_at < datetime('now','-1825 days')"));
    q.exec();
}
