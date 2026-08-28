#pragma once

#include <QWidget>

#include "core/models/AppSettings.h"

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLineEdit;
class QLabel;
class SettingsService;
class QPushButton;
class QDoubleSpinBox;

// 设置页：币种、刷新间隔、配色、通知、备份。
class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(SettingsService *service, QWidget *parent = nullptr);

signals:
    void settingsSaved(const AppSettings &settings);

private:
    void loadFrom(const AppSettings &settings);
    void save();

    SettingsService *m_service = nullptr;
    QComboBox *m_currency = nullptr;
    QSpinBox *m_interval = nullptr;
    QComboBox *m_colorScheme = nullptr;
    QCheckBox *m_notifications = nullptr;
    QCheckBox *m_trayOnClose = nullptr;
    QSpinBox *m_requestMs = nullptr;
    QDoubleSpinBox *m_feeSteam = nullptr;
    QDoubleSpinBox *m_feeGame = nullptr;
    QComboBox *m_game = nullptr;
    QComboBox *m_startupIdentity = nullptr;
    QComboBox *m_proxyMode = nullptr;
    QLineEdit *m_proxyHost = nullptr;
    QSpinBox *m_proxyPort = nullptr;
    QLabel *m_proxyHint = nullptr;
    QPushButton *m_backupBtn = nullptr;
};
