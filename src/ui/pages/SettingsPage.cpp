#include "ui/pages/SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "core/services/SettingsService.h"

SettingsPage::SettingsPage(SettingsService *service, QWidget *parent)
    : QWidget(parent), m_service(service) {
    m_currency = new QComboBox(this);
    m_currency->addItems({QStringLiteral("CNY"), QStringLiteral("USD"), QStringLiteral("EUR"),
                          QStringLiteral("RUB")});
    m_game = new QComboBox(this);
    m_game->addItem(QStringLiteral("CS2"), 730);
    m_game->addItem(QStringLiteral("DOTA2"), 570);
    m_game->addItem(QStringLiteral("军团要塞2（预留）"), 440);
    m_startupIdentity = new QComboBox(this);
    m_startupIdentity->addItem(QStringLiteral("每次询问"), QStringLiteral("ask"));
    m_startupIdentity->addItem(QStringLiteral("直接以游客进入"), QStringLiteral("guest"));
    if (auto *model = qobject_cast<QStandardItemModel *>(m_game->model())) {
        if (QStandardItem *tf2Item = model->item(2)) {
            tf2Item->setEnabled(false);
        }
    }
    m_interval = new QSpinBox(this);
    m_interval->setRange(1, 1440);
    m_interval->setSuffix(QStringLiteral(" 分钟"));
    m_colorScheme = new QComboBox(this);
    m_colorScheme->addItems({QStringLiteral("红涨绿跌（A股习惯）"),
                             QStringLiteral("绿涨红跌（国际习惯）")});
    m_notifications = new QCheckBox(QStringLiteral("启用通知"), this);
    m_trayOnClose = new QCheckBox(QStringLiteral("关闭窗口时驻留托盘"), this);
    m_requestMs = new QSpinBox(this);
    m_requestMs->setRange(500, 10000);
    m_requestMs->setSingleStep(100);
    m_requestMs->setSuffix(QStringLiteral(" ms"));
    m_feeSteam = new QDoubleSpinBox(this);
    m_feeSteam->setRange(0, 50);
    m_feeSteam->setDecimals(2);
    m_feeSteam->setSuffix(QStringLiteral(" %"));
    m_feeSteam->setValue(5.0);
    m_feeGame = new QDoubleSpinBox(this);
    m_feeGame->setRange(0, 50);
    m_feeGame->setDecimals(2);
    m_feeGame->setSuffix(QStringLiteral(" %"));
    m_feeGame->setValue(10.0);

    auto *saveBtn = new QPushButton(QStringLiteral("保存设置"), this);
    m_backupBtn = new QPushButton(QStringLiteral("立即备份数据库"), this);
    auto *hint = new QLabel(
        QStringLiteral("第三方比价支持 CSV 导入（market_hash_name,platform,price,currency,url）"), this);
    hint->setWordWrap(true);

    auto *form = new QFormLayout();
    form->addRow(QStringLiteral("默认游戏"), m_game);
    form->addRow(QStringLiteral("启动身份"), m_startupIdentity);
    form->addRow(QStringLiteral("币种"), m_currency);
    form->addRow(QStringLiteral("自动刷新间隔"), m_interval);
    form->addRow(QStringLiteral("涨跌配色"), m_colorScheme);
    form->addRow(m_notifications);
    form->addRow(m_trayOnClose);
    form->addRow(QStringLiteral("接口请求间隔"), m_requestMs);
    form->addRow(QStringLiteral("费用·Steam 费率"), m_feeSteam);
    form->addRow(QStringLiteral("费用·游戏费率"), m_feeGame);
    form->addRow(saveBtn);
    form->addRow(m_backupBtn);
    form->addRow(hint);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addStretch();

    connect(m_service, &SettingsService::settingsChanged, this, &SettingsPage::loadFrom);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::save);
    connect(m_backupBtn, &QPushButton::clicked, this, [this]() {
        emit settingsSaved(m_service->settings());  // AppController 订阅并执行备份
        QMessageBox::information(this, QStringLiteral("备份"), QStringLiteral("数据库备份完成"));
    });
    loadFrom(m_service->settings());
}

void SettingsPage::loadFrom(const AppSettings &settings) {
    const int gameIdx = m_game->findData(settings.gameAppid);
    if (gameIdx >= 0) {
        m_game->setCurrentIndex(gameIdx);
    }
    m_startupIdentity->setCurrentIndex(
        settings.startupIdentityMode == AppSettings::StartupIdentityMode::kGuest ? 1 : 0);
    m_currency->setCurrentText(settings.currency);
    m_interval->setValue(settings.refreshIntervalMinutes);
    m_colorScheme->setCurrentIndex(settings.colorScheme == AppSettings::ColorScheme::kGreenUpRedDown
                                       ? 1
                                       : 0);
    m_notifications->setChecked(settings.notificationsEnabled);
    m_trayOnClose->setChecked(settings.trayOnClose);
    m_requestMs->setValue(settings.requestIntervalMs);
    m_feeSteam->setValue(settings.feeSteamRate * 100.0);
    m_feeGame->setValue(settings.feeGameRate * 100.0);
}

void SettingsPage::save() {
    AppSettings next = m_service->settings();
    next.gameAppid = m_game->currentData().toInt();
    next.startupIdentityMode = m_startupIdentity->currentData().toString() == QLatin1String("guest")
                                   ? AppSettings::StartupIdentityMode::kGuest
                                   : AppSettings::StartupIdentityMode::kAskEveryTime;
    next.currency = m_currency->currentText();
    next.refreshIntervalMinutes = m_interval->value();
    next.colorScheme = m_colorScheme->currentIndex() == 1
                           ? AppSettings::ColorScheme::kGreenUpRedDown
                           : AppSettings::ColorScheme::kRedUpGreenDown;
    next.notificationsEnabled = m_notifications->isChecked();
    next.trayOnClose = m_trayOnClose->isChecked();
    next.requestIntervalMs = m_requestMs->value();
    next.feeSteamRate = m_feeSteam->value() / 100.0;
    next.feeGameRate = m_feeGame->value() / 100.0;
    if (m_service->save(next)) {
        emit settingsSaved(next);
        QMessageBox::information(this, QStringLiteral("设置已保存"),
                                 QStringLiteral("设置已生效并持久化"));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("设置校验失败，请检查输入"));
    }
}
