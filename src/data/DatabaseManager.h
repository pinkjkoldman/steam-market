#pragma once

#include <QString>

class QSqlDatabase;

// SQLite 数据库连接管理：打开、WAL、外键、版本化迁移、备份。
class DatabaseManager {
public:
    explicit DatabaseManager(const QString &dbPath);
    ~DatabaseManager();

    // 打开并执行未应用迁移；失败返回 false。
    bool open();
    void close();
    bool isOpen() const;
    QString dbPath() const { return m_dbPath; }
    QSqlDatabase database() const;

    // 备份：checkpoint 后复制主库到 .bak。
    bool backup();
    // 清理过期快照（90 天）与超长历史（5 年）。
    void runRetentionPolicy();

private:
    bool migrate();
    QString m_dbPath;
    QString m_connectionName;
};
