#include "ui/pages/SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

#include "core/services/SettingsService.h"
#include "utils/AppInfo.h"

SettingsPage::SettingsPage(SettingsService *service, QWidget *parent)
    : QWidget(parent), m_service(service) {
    setMinimumHeight(620);
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
    m_proxyMode = new QComboBox(this);
    m_proxyMode->addItem(QStringLiteral("跟随系统"), QStringLiteral("system"));
    m_proxyMode->addItem(QStringLiteral("直连（不使用代理）"), QStringLiteral("disabled"));
    m_proxyMode->addItem(QStringLiteral("HTTP 代理"), QStringLiteral("http"));
    m_proxyMode->addItem(QStringLiteral("SOCKS5 代理"), QStringLiteral("socks5"));
    m_proxyHost = new QLineEdit(this);
    m_proxyHost->setPlaceholderText(QStringLiteral("如 127.0.0.1"));
    m_proxyPort = new QSpinBox(this);
    m_proxyPort->setRange(1, 65535);
    m_proxyPort->setValue(1080);
    m_proxyHint = new QLabel(QStringLiteral("代理无法连接 Steam 时可切换到直连或其它模式，保存后立即生效"), this);
    m_proxyHint->setWordWrap(true);
    m_proxyHint->setObjectName(QStringLiteral("mutedText"));

    auto *saveBtn = new QPushButton(QStringLiteral("保存设置"), this);
    m_backupBtn = new QPushButton(QStringLiteral("立即备份数据库"), this);
    auto *hint = new QLabel(
        QStringLiteral("第三方比价支持 CSV 导入（market_hash_name,platform,price,currency,url）"), this);
    hint->setWordWrap(true);

    // 关于
    auto *aboutLabel = new QLabel(
        QStringLiteral("Steam 行情终端 v" APP_VERSION
                       "\nWindows 桌面应用 · Qt 6 · C++17"
                       "\n\n本软件为非官方工具，行情数据来自 Steam 社区市场公开接口，"
       "仅供个人参考，不构成任何交易建议。"
                       "\n软件不收集、不上传任何用户数据；登录仅用于获取自己的库存与官方历史价格。"),
        this);
    aboutLabel->setWordWrap(true);
    auto *openDataDirBtn = new QPushButton(QStringLiteral("打开数据目录"), this);
    auto *openLogDirBtn = new QPushButton(QStringLiteral("打开日志目录"), this);
    auto *aboutActions = new QHBoxLayout();
    aboutActions->addWidget(openDataDirBtn);
    aboutActions->addWidget(openLogDirBtn);
    aboutActions->addStretch();

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
    form->addRow(QStringLiteral("网络代理"), m_proxyMode);
    form->addRow(QStringLiteral("代理地址"), m_proxyHost);
    form->addRow(QStringLiteral("代理端口"), m_proxyPort);
    form->addRow(m_proxyHint);
    form->addRow(saveBtn);
    form->addRow(m_backupBtn);
    form->addRow(hint);
    form->addRow(QStringLiteral("关于"), aboutLabel);
    form->addRow(aboutActions);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("pageCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);
    cardLayout->addLayout(form);
    cardLayout->addStretch();

    layout->addWidget(card, 1);

    connect(m_service, &SettingsService::settingsChanged, this, &SettingsPage::loadFrom);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::save);
    connect(m_proxyMode, &QComboBox::currentIndexChanged, this, [this](int index) {
        const bool manual = index == 2 || index == 3;  // HTTP / SOCKS5
        m_proxyHost->setEnabled(manual);
        m_proxyPort->setEnabled(manual);
    });
    connect(m_backupBtn, &QPushButton::clicked, this, [this]() {
        emit settingsSaved(m_service->settings());  // AppController 订阅并执行备份
        QMessageBox::information(this, QStringLiteral("备份"), QStringLiteral("数据库备份完成"));
    });
    connect(openDataDirBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QStandardPaths::writableLocation(
                QStandardPaths::AppDataLocation)));
    });
    connect(openLogDirBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QStandardPaths::writableLocation(
                QStandardPaths::AppDataLocation) + QStringLiteral("/logs")));
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
    m_proxyMode->setCurrentIndex(static_cast<int>(settings.proxyMode));
    m_proxyHost->setText(settings.proxyHost);
    m_proxyPort->setValue(settings.proxyPort);
    const bool manualProxy = settings.proxyMode == AppSettings::ProxyMode::kHttp
                             || settings.proxyMode == AppSettings::ProxyMode::kSocks5;
    m_proxyHost->setEnabled(manualProxy);
    m_proxyPort->setEnabled(manualProxy);
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
    next.proxyMode = static_cast<AppSettings::ProxyMode>(m_proxyMode->currentIndex());
    next.proxyHost = m_proxyHost->text().trimmed();
    next.proxyPort = m_proxyPort->value();
    if ((next.proxyMode == AppSettings::ProxyMode::kHttp
         || next.proxyMode == AppSettings::ProxyMode::kSocks5)
        && next.proxyHost.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("代理配置不完整"),
                             QStringLiteral("选择了手动代理但未填写代理地址"));
        return;
    }
    if (m_service->save(next)) {
        emit settingsSaved(next);
        QMessageBox::information(this, QStringLiteral("设置已保存"),
                                 QStringLiteral("设置已生效并持久化"));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("设置校验失败，请检查输入"));
    }
}
