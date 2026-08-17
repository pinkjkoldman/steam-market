#pragma once

#include <QString>

// 全局日志：控制台 + 文件（%APPDATA%/SteamMarketTerminal/logs/app.log，滚动）。
namespace Logger {
void init();
void setLogDir(const QString &dir);
QString logFilePath();
}  // namespace Logger
