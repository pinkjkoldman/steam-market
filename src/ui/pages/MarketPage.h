#pragma once

#include <QPixmap>
#include <QWidget>

#include "core/models/MarketItem.h"

class QLineEdit;
class QComboBox;
class QFrame;
class QTableView;
class QStandardItemModel;
class QLabel;
class QPushButton;
class MarketService;

// 行情页：搜索 + 结果列表。
class MarketPage : public QWidget {
    Q_OBJECT

public:
    explicit MarketPage(MarketService *service, QWidget *parent = nullptr);
    void setGame(int appid);

signals:
    void itemSelected(const MarketItem &item);

private:
    void runSearch();
    void loadMore();
    void openCurrentItem();
    void render(const QVector<MarketItem> &items, int totalCount, const QString &error);
    void appendItems(const QVector<MarketItem> &items);
    void updateMoreButton();
    void onIconReady(const QString &iconPath, const QPixmap &pixmap);
    void updateEmptyState(bool empty);

    MarketService *m_service = nullptr;
    QComboBox *m_game = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_moreBtn = nullptr;
    QLineEdit *m_search = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_emptyIcon = nullptr;
    QLabel *m_emptyTitle = nullptr;
    QLabel *m_emptySubtitle = nullptr;
    QFrame *m_card = nullptr;
    QWidget *m_emptyState = nullptr;
    class LoadingOverlay *m_loading = nullptr;
    QString m_query;
    int m_appid = 730;
    int m_total = 0;
    bool m_appending = false;
};
