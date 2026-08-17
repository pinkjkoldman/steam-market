#pragma once

#include <QObject>

#include "core/models/AppSettings.h"

class SettingsRepository;

// 设置读写：DB 持久化 + 内存缓存；非法值回退默认。
class SettingsService : public QObject {
    Q_OBJECT

public:
    explicit SettingsService(SettingsRepository *repo, QObject *parent = nullptr);

    AppSettings settings() const { return m_settings; }
    bool save(const AppSettings &next);

signals:
    void settingsChanged(const AppSettings &settings);

private:
    AppSettings load() const;
    SettingsRepository *m_repo = nullptr;
    AppSettings m_settings;
};
