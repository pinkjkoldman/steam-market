#pragma once

#include <QObject>

class QSystemTrayIcon;
class QMenu;

// 系统托盘：常驻、气泡通知、显示/退出。
class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(QObject *parent = nullptr);
    ~TrayManager() override;

    bool isAvailable() const;
    void showMessage(const QString &title, const QString &body);
    void showIcon();

signals:
    void showRequested();
    void quitRequested();

private:
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
};
