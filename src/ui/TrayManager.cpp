#include "ui/TrayManager.h"

#include "utils/AppInfo.h"

#include <QApplication>
#include <QMenu>
#include <QStyle>
#include <QSystemTrayIcon>

TrayManager::TrayManager(QObject *parent) : QObject(parent) {
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon(QStringLiteral(":/ui/app.png")));
    m_tray->setToolTip(QStringLiteral("Steam 行情终端 v" APP_VERSION));

    m_menu = new QMenu();
    QAction *showAction = m_menu->addAction(QStringLiteral("显示主窗口"));
    QAction *quitAction = m_menu->addAction(QStringLiteral("退出"));
    connect(showAction, &QAction::triggered, this, &TrayManager::showRequested);
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);
    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            emit showRequested();
        }
    });
}

TrayManager::~TrayManager() {
    delete m_menu;
}

bool TrayManager::isAvailable() const {
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayManager::showMessage(const QString &title, const QString &body) {
    if (isAvailable()) {
        m_tray->showMessage(title, body, QSystemTrayIcon::Information, 8000);
    }
}

void TrayManager::showIcon() {
    if (isAvailable()) {
        m_tray->show();
    }
}
