#pragma once

#include <QWidget>

class QComboBox;
class QTableView;
class QStandardItemModel;
class QLabel;
class QFrame;
class LoadingOverlay;
class RankingService;

// 排行榜页：成交量/涨幅/跌幅榜。
class RankingPage : public QWidget {
    Q_OBJECT

public:
    explicit RankingPage(RankingService *service, QWidget *parent = nullptr);

signals:
    void itemSelected(const QString &marketHashName, int appid);

private:
    void reload();
    void updateEmptyState(bool empty);

    RankingService *m_service = nullptr;
    QComboBox *m_by = nullptr;
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
