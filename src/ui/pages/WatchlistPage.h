#pragma once

#include <QWidget>

class QTableView;
class QStandardItemModel;
class QLabel;
class QFrame;
class LoadingOverlay;
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
    void updateEmptyState(bool empty);

    WatchlistService *m_watchlist = nullptr;
    AlertService *m_alerts = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_status = nullptr;
    QFrame *m_card = nullptr;
    QWidget *m_emptyState = nullptr;
    QLabel *m_emptyIcon = nullptr;
    QLabel *m_emptyTitle = nullptr;
    QLabel *m_emptySubtitle = nullptr;
    LoadingOverlay *m_loading = nullptr;
};
