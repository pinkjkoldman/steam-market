#include "ui/widgets/OrderbookPanel.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
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

qint64 topDepth(const QVector<OrderbookEntry> &entries) {
    qint64 total = 0;
    const int count = qMin(5, entries.size());
    for (int index = 0; index < count; ++index) total += entries.at(index).count;
    return total;
}
}  // namespace

OrderbookPanel::OrderbookPanel(QWidget *parent) : QWidget(parent) {
    m_best = new QLabel(QStringLiteral("最高买价 — / 最低卖价 —"), this);
    m_best->setObjectName(QStringLiteral("sectionTitle"));
    m_analysis = new QLabel(QStringLiteral("价差与深度分析将在盘口加载后显示"), this);
    m_analysis->setObjectName(QStringLiteral("mutedText"));
    m_analysis->setWordWrap(true);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("mutedText"));
    m_status->setWordWrap(true);

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
    layout->addWidget(m_analysis);
    layout->addLayout(tables);
    layout->addWidget(m_status);
}

void OrderbookPanel::setOrderbook(const Orderbook &book, const QString &errorMessage) {
    const bool hasData = !book.buyOrders.isEmpty() || !book.sellOrders.isEmpty();
    const QString responseCurrency = Currency::codeFromSteamId(book.steamCurrencyId);
    const QString currency = responseCurrency.isEmpty() ? CurrencyProvider::code()
                                                        : responseCurrency;
    const QString symbol = Currency::displaySymbol(currency);
    m_best->setText(QStringLiteral("最高买价 %1 / 最低卖价 %2")
                        .arg(book.highestBuy > 0 ? symbol + QString::number(book.highestBuy, 'f', 2)
                                                 : QStringLiteral("—"))
                        .arg(book.lowestSell > 0 ? symbol + QString::number(book.lowestSell, 'f', 2)
                                                 : QStringLiteral("—")));
    const qint64 buyDepth = topDepth(book.buyOrders);
    const qint64 sellDepth = topDepth(book.sellOrders);
    if (book.highestBuy > 0 && book.lowestSell > 0) {
        const double spread = book.lowestSell - book.highestBuy;
        const double midpoint = (book.lowestSell + book.highestBuy) / 2.0;
        const double spreadPercent = midpoint > 0 ? spread / midpoint * 100.0 : 0.0;
        const qint64 depthTotal = buyDepth + sellDepth;
        const double imbalance = depthTotal > 0
                                     ? static_cast<double>(buyDepth - sellDepth)
                                           / static_cast<double>(depthTotal) * 100.0
                                     : 0.0;
        m_analysis->setText(
            QStringLiteral("买卖价差 %1%2（%3%） · 前 5 档深度 买 %4 / 卖 %5 · 失衡率 %6%7%")
                .arg(symbol)
                .arg(spread, 0, 'f', 2)
                .arg(spreadPercent, 0, 'f', 2)
                .arg(QLocale().toString(buyDepth))
                .arg(QLocale().toString(sellDepth))
                .arg(imbalance >= 0 ? QStringLiteral("+") : QString())
                .arg(imbalance, 0, 'f', 1));
    } else {
        m_analysis->setText(QStringLiteral("当前数据不足以计算买卖价差与失衡率"));
    }
    fillTable(m_buyTable, book.buyOrders);
    fillTable(m_sellTable, book.sellOrders);
    if (!errorMessage.isEmpty()) {
        m_status->setText(errorMessage + (hasData ? QStringLiteral("（展示缓存数据）") : QString()));
    } else if (hasData) {
        const QString fetchedAt = book.fetchedAt.isValid()
                                      ? QLocale().toString(book.fetchedAt.toLocalTime(),
                                                           QLocale::ShortFormat)
                                      : QStringLiteral("未知");
        m_status->setText(
            book.stale
                ? QStringLiteral("离线模式：展示缓存盘口 · 币种 %1 · 抓取 %2")
                      .arg(currency, fetchedAt)
                : QStringLiteral("数据来源：Steam 新版官方盘口 · 币种 %1 · 更新 %2")
                      .arg(currency, fetchedAt));
    } else {
        m_status->setText(QStringLiteral("暂无盘口数据"));
    }
}
