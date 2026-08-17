#include "ui/widgets/OrderbookPanel.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"

namespace {
void fillTable(QTableWidget *table, const QVector<OrderbookEntry> &entries) {
    table->setRowCount(5);
    const int count = qMin(5, entries.size());
    for (int i = 0; i < 5; ++i) {
        if (i < count) {
            const OrderbookEntry &e = entries.at(i);
            table->setItem(i, 0, new QTableWidgetItem(QStringLiteral("%1").arg(e.price, 0, 'f', 2)));
            table->setItem(i, 1, new QTableWidgetItem(QString::number(e.count)));
        } else {
            table->setItem(i, 0, new QTableWidgetItem(QStringLiteral("—")));
            table->setItem(i, 1, new QTableWidgetItem(QStringLiteral("—")));
        }
    }
}
}  // namespace

OrderbookPanel::OrderbookPanel(QWidget *parent) : QWidget(parent) {
    m_best = new QLabel(QStringLiteral("最高买价 — / 最低卖价 —"), this);
    m_status = new QLabel(this);

    m_buyTable = new QTableWidget(5, 2, this);
    m_sellTable = new QTableWidget(5, 2, this);
    for (QTableWidget *t : {m_buyTable, m_sellTable}) {
        t->setHorizontalHeaderLabels({QStringLiteral("价格"), QStringLiteral("数量")});
        t->horizontalHeader()->setStretchLastSection(true);
        t->verticalHeader()->setVisible(false);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setFixedHeight(180);
    }
    auto *buyLabel = new QLabel(QStringLiteral("买盘（求购）"), this);
    auto *sellLabel = new QLabel(QStringLiteral("卖盘（挂单）"), this);
    buyLabel->setStyleSheet(QStringLiteral("color: #E5484D; font-weight: bold;"));
    sellLabel->setStyleSheet(QStringLiteral("color: #46A758; font-weight: bold;"));

    auto *buyBox = new QVBoxLayout();
    buyBox->addWidget(buyLabel);
    buyBox->addWidget(m_buyTable);
    auto *sellBox = new QVBoxLayout();
    sellBox->addWidget(sellLabel);
    sellBox->addWidget(m_sellTable);
    auto *tables = new QHBoxLayout();
    tables->addLayout(buyBox, 1);
    tables->addLayout(sellBox, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_best);
    layout->addLayout(tables);
    layout->addWidget(m_status);
}

void OrderbookPanel::setOrderbook(const Orderbook &book, const QString &errorMessage) {
    const bool hasData = !book.buyOrders.isEmpty() || !book.sellOrders.isEmpty();
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    m_best->setText(QStringLiteral("最高买价 %1 / 最低卖价 %2")
                        .arg(book.highestBuy > 0 ? symbol + QString::number(book.highestBuy, 'f', 2)
                                                 : QStringLiteral("—"))
                        .arg(book.lowestSell > 0 ? symbol + QString::number(book.lowestSell, 'f', 2)
                                                 : QStringLiteral("—")));
    fillTable(m_buyTable, book.buyOrders);
    fillTable(m_sellTable, book.sellOrders);
    if (!errorMessage.isEmpty()) {
        m_status->setText(errorMessage + (hasData ? QStringLiteral("（展示缓存数据）") : QString()));
    } else if (hasData) {
        m_status->setText(book.stale ? QStringLiteral("离线模式：展示缓存盘口")
                                     : QStringLiteral("盘口已更新"));
    } else {
        m_status->setText(QStringLiteral("暂无盘口数据"));
    }
}
