#include "ui/widgets/IconCache.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QtDebug>

namespace {
const QString kImageBase = QStringLiteral("https://community.steam-api.com/economy/image/");

QString fileNameFor(const QString &iconPath) {
    const QByteArray digest =
        QCryptographicHash::hash(iconPath.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(digest) + QStringLiteral(".png");
}
}  // namespace

IconCache::IconCache(QObject *parent) : QObject(parent) {
    m_nam = new QNetworkAccessManager(this);  // 跟随应用级代理设置
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_cacheDir = base + QStringLiteral("/icons");
    QDir().mkpath(m_cacheDir);
}

IconCache *IconCache::instance() {
    static IconCache cache;
    return &cache;
}

QPixmap IconCache::pixmap(const QString &iconPath) {
    const QString key = iconPath.trimmed();
    if (key.isEmpty()) return QPixmap();
    const auto it = m_memory.constFind(key);
    if (it != m_memory.constEnd()) return it.value();
    const QPixmap disk = loadFromDisk(key);
    if (!disk.isNull()) {
        m_memory.insert(key, disk);
        return disk;
    }
    fetch(key);
    return QPixmap();
}

QPixmap IconCache::loadFromDisk(const QString &iconPath) {
    QPixmap pix;
    const QString path = m_cacheDir + QLatin1Char('/') + fileNameFor(iconPath);
    if (QFile::exists(path) && pix.load(path)) {
        return pix.scaledToHeight(48, Qt::SmoothTransformation);
    }
    return QPixmap();
}

void IconCache::fetch(const QString &iconPath) {
    if (m_inFlight.contains(iconPath)) return;
    m_inFlight.insert(iconPath);
    QNetworkRequest request(QUrl(kImageBase + iconPath));
    request.setTransferTimeout(10000);
    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, iconPath]() {
        reply->deleteLater();
        m_inFlight.remove(iconPath);
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "图标下载失败:" << iconPath << reply->errorString();
            return;
        }
        const QByteArray bytes = reply->readAll();
        const QString path = m_cacheDir + QLatin1Char('/') + fileNameFor(iconPath);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(bytes);
            file.close();
        }
        QPixmap pix;
        if (!pix.loadFromData(bytes)) return;
        const QPixmap scaled = pix.scaledToHeight(48, Qt::SmoothTransformation);
        m_memory.insert(iconPath, scaled);
        emit ready(iconPath, scaled);
    });
}
