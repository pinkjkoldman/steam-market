#pragma once

#include <QWidget>

#include "core/models/Orderbook.h"

class QLabel;
class QTableWidget;

// 买卖盘口面板：最优价 + 买/卖前 5 档挂单。
class OrderbookPanel : public QWidget {
    Q_OBJECT

public:
    explicit OrderbookPanel(QWidget *parent = nullptr);

public slots:
    void setOrderbook(const Orderbook &book, const QString &errorMessage);

private:
    QLabel *m_best = nullptr;
    QLabel *m_status = nullptr;
    QTableWidget *m_buyTable = nullptr;
    QTableWidget *m_sellTable = nullptr;
};
