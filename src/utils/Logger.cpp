#include "utils/Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageLogContext>
#include <QStandardPaths>
#include <QtGlobal>

namespace {
QFile *g_logFile = nullptr;
QString g_logDir;

void rotateIfNeeded(QFile *file) {
    // 每个文件上限 5MB，保留最近 2 个滚动副本。
    const qint64 kMaxBytes = 5 * 1024 * 1024;
    if (file->size() <= kMaxBytes) {
        return;
    }
    const QString base = Logger::logFilePath();
    file->close();
    QFile::remove(base + QStringLiteral(".1"));
    QFile::rename(base, base + QStringLiteral(".1"));
    file->setFileName(base);
    file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    if (!g_logFile) {
        return;
    }
    const char *level = "INFO";
    switch (type) {
        case QtDebugMsg: level = "DEBUG"; break;
        case QtInfoMsg: level = "INFO"; break;
        case QtWarningMsg: level = "WARN"; break;
        case QtCriticalMsg: level = "ERROR"; break;
        case QtFatalMsg: level = "FATAL"; break;
    }
    const QString line = QStringLiteral("%1 [%2] %3 (%4:%5)\n")
                             .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                                  QString::fromLatin1(level), msg,
                                  QString::fromUtf8(context.file ? context.file : "?"))
                             .arg(context.line);
    g_logFile->write(line.toUtf8());
    g_logFile->flush();
    rotateIfNeeded(g_logFile);
}
}  // namespace

namespace Logger {

void setLogDir(const QString &dir) {
    g_logDir = dir;
}

QString logFilePath() {
    return g_logDir + QStringLiteral("/app.log");
}

void init() {
    if (g_logFile) {
        return;
    }
    if (g_logDir.isEmpty()) {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        g_logDir = base + QStringLiteral("/logs");
    }
    QDir().mkpath(g_logDir);
    g_logFile = new QFile(logFilePath());
    g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler(messageHandler);
}

}  // namespace Logger
