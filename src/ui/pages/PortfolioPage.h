#pragma once

#include <QWidget>

class QTableView;
class QStandardItemModel;
class QLabel;
class QFrame;
class LoadingOverlay;
class PortfolioService;
class TradeSimulationService;

// 持仓页：明细表 + 汇总卡片 + 增删改。
class PortfolioPage : public QWidget {
    Q_OBJECT

public:
    explicit PortfolioPage(PortfolioService *service, TradeSimulationService *trades,
                           QWidget *parent = nullptr);

signals:
    void itemSelected(const QString &marketHashName, int appid);

private:
    void reload();
    void addEdit(int editId);
    void openTrade(bool buy);
    void showTrades();
    void updateEmptyState(bool empty);

    PortfolioService *m_service = nullptr;
    TradeSimulationService *m_trades = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_summary = nullptr;
    QLabel *m_status = nullptr;
    QFrame *m_card = nullptr;
    QWidget *m_emptyState = nullptr;
    QLabel *m_emptyIcon = nullptr;
    QLabel *m_emptyTitle = nullptr;
    QLabel *m_emptySubtitle = nullptr;
    LoadingOverlay *m_loading = nullptr;
};
