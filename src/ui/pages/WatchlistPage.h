#pragma once

#include <QWidget>

class QTableView;
class QStandardItemModel;
class QLabel;
class WatchlistService;
class AlertService;

// 自选页：关注列表、刷新、移除、提醒、跳详情。
class WatchlistPage : public QWidget {
    Q_OBJECT

public:
    WatchlistPage(WatchlistService *watchlist, AlertService *alerts, QWidget *parent = nullptr);

signals:
    void itemSelected(const QString &marketHashName, int appid);

private:
    void reload();
    void setupAlertForSelected();

    WatchlistService *m_watchlist = nullptr;
    AlertService *m_alerts = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_status = nullptr;
};
