#pragma once

#include <QWidget>

#include "core/models/MarketItem.h"

class QLineEdit;
class QComboBox;
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
    void openCurrentItem();
    void render(const QVector<MarketItem> &items, const QString &error);

    MarketService *m_service = nullptr;
    QComboBox *m_game = nullptr;
    QPushButton *m_searchButton = nullptr;
    QLineEdit *m_search = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_status = nullptr;
};
