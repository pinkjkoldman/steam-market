#pragma once

#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QSet>

class QNetworkAccessManager;

// Steam 市场物品图标缓存：内存 → 磁盘 → 网络三级回源。
// iconPath 为 Steam 搜索接口返回的 asset_description.icon_url（相对路径），
// 完整地址为 https://community.steam-api.com/economy/image/<iconPath>。
// 下载完成（或磁盘命中）后发出 ready 信号，供表格异步补画图标。
class IconCache : public QObject {
    Q_OBJECT

public:
    static IconCache *instance();

    // 同步取图标：内存/磁盘命中直接返回，否则触发后台下载并返回空图。
    QPixmap pixmap(const QString &iconPath);

signals:
    void ready(const QString &iconPath, const QPixmap &pixmap);

private:
    explicit IconCache(QObject *parent = nullptr);
    QPixmap loadFromDisk(const QString &iconPath);
    void fetch(const QString &iconPath);

    QNetworkAccessManager *m_nam = nullptr;
    QHash<QString, QPixmap> m_memory;
    QSet<QString> m_inFlight;
    QString m_cacheDir;
};
