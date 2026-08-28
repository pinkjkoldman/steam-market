#include "ui/pages/PortfolioPage.h"

#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/services/PortfolioService.h"
#include "core/services/TradeSimulationService.h"
#include "ui/widgets/LoadingOverlay.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"
#include "utils/ThemeProvider.h"

PortfolioPage::PortfolioPage(PortfolioService *service, TradeSimulationService *trades,
                             QWidget *parent)
    : QWidget(parent), m_service(service), m_trades(trades) {
    auto *addBtn = new QPushButton(QStringLiteral("添加物品"), this);
    auto *editBtn = new QPushButton(QStringLiteral("编辑选中"), this);
    auto *delBtn = new QPushButton(QStringLiteral("删除选中"), this);
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新估值"), this);
    auto *buyBtn = new QPushButton(QStringLiteral("模拟买入"), this);
    auto *sellBtn = new QPushButton(QStringLiteral("模拟卖出"), this);
    auto *recordsBtn = new QPushButton(QStringLiteral("交易记录"), this);
    auto *topRow = new QHBoxLayout();
    topRow->addWidget(addBtn);
    topRow->addWidget(editBtn);
    topRow->addWidget(delBtn);
    topRow->addWidget(refreshBtn);
    topRow->addWidget(buyBtn);
    topRow->addWidget(sellBtn);
    topRow->addWidget(recordsBtn);
    topRow->addStretch();

    m_table = new QTableView(this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(
        {QStringLiteral("物品名称"), QStringLiteral("数量"), QStringLiteral("成本价"),
         QStringLiteral("最新价"), QStringLiteral("市值"), QStringLiteral("盈亏"),
         QStringLiteral("盈亏率")});
    m_table->setModel(m_model);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_summary = new QLabel(this);
    m_summary->setStyleSheet(QStringLiteral("font-weight: bold; padding: 6px;"));
    m_status = new QLabel(this);

    m_card = new QFrame(this);
    m_card->setObjectName(QStringLiteral("pageCard"));
    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);
    cardLayout->addLayout(topRow);
    cardLayout->addWidget(m_summary);
    cardLayout->addWidget(m_table, 1);
    cardLayout->addWidget(m_status);

    m_emptyState = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(8);
    m_emptyIcon = new QLabel(QStringLiteral("📊"), m_emptyState);
    m_emptyIcon->setObjectName(QStringLiteral("emptyStateIcon"));
    m_emptyIcon->setAlignment(Qt::AlignCenter);
    m_emptyTitle = new QLabel(QStringLiteral("暂无持仓记录"), m_emptyState);
    m_emptyTitle->setObjectName(QStringLiteral("emptyStateTitle"));
    m_emptyTitle->setAlignment(Qt::AlignCenter);
    m_emptySubtitle = new QLabel(QStringLiteral("点击“添加物品”记录成本，或“模拟买入”生成交易记录"), m_emptyState);
    m_emptySubtitle->setObjectName(QStringLiteral("emptyStateSubtitle"));
    m_emptySubtitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(m_emptyIcon);
    emptyLayout->addWidget(m_emptyTitle);
    emptyLayout->addWidget(m_emptySubtitle);

    auto *viewStack = new QStackedWidget(this);
    viewStack->addWidget(m_card);
    viewStack->addWidget(m_emptyState);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(viewStack, 1);

    m_loading = new LoadingOverlay(this);
    m_loading->setText(QStringLiteral("正在刷新估值…"));

    connect(addBtn, &QPushButton::clicked, this, [this]() { addEdit(0); });
    connect(editBtn, &QPushButton::clicked, this, [this]() {
        const QModelIndex idx = m_table->currentIndex();
        if (idx.isValid()) {
            addEdit(m_model->item(idx.row(), 0)->data(Qt::UserRole).toInt());
        }
    });
    connect(delBtn, &QPushButton::clicked, this, [this]() {
        const QModelIndex idx = m_table->currentIndex();
        if (!idx.isValid()) return;
        const int id = m_model->item(idx.row(), 0)->data(Qt::UserRole).toInt();
        if (QMessageBox::question(this, QStringLiteral("确认"),
                                  QStringLiteral("删除该持仓？"))
            == QMessageBox::Yes) {
            m_service->remove(id);
        }
    });
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        m_loading->show();
        m_service->refreshPrices();
    });
    connect(buyBtn, &QPushButton::clicked, this, [this]() { openTrade(true); });
    connect(sellBtn, &QPushButton::clicked, this, [this]() { openTrade(false); });
    connect(recordsBtn, &QPushButton::clicked, this, &PortfolioPage::showTrades);
    connect(m_trades, &TradeSimulationService::tradesChanged, m_service,
            &PortfolioService::refreshPrices);
    connect(m_table, &QTableView::doubleClicked, this, [this]() {
        const QModelIndex idx = m_table->currentIndex();
        if (idx.isValid()) {
            emit itemSelected(m_model->item(idx.row(), 0)->data(Qt::UserRole + 1).toString(),
                              m_model->item(idx.row(), 0)->data(Qt::UserRole + 2).toInt());
        }
    });
    connect(m_service, &PortfolioService::portfolioChanged, this, &PortfolioPage::reload);
}

void PortfolioPage::updateEmptyState(bool empty) {
    auto *stack = qobject_cast<QStackedWidget *>(layout()->itemAt(0)->widget());
    if (stack) stack->setCurrentIndex(empty ? 1 : 0);
}

void PortfolioPage::openTrade(bool buy) {
    QDialog dlg(this);
    dlg.setWindowTitle(buy ? QStringLiteral("模拟买入") : QStringLiteral("模拟卖出"));
    auto *form = new QFormLayout(&dlg);
    QString selectedName;
    const QModelIndex idx = m_table->currentIndex();
    if (idx.isValid()) {
        selectedName = m_model->item(idx.row(), 0)->data(Qt::UserRole + 1).toString();
    }
    auto *nameEdit = new QLineEdit(selectedName, &dlg);
    auto *qtySpin = new QSpinBox(&dlg);
    qtySpin->setRange(1, 100000);
    auto *priceSpin = new QDoubleSpinBox(&dlg);
    priceSpin->setRange(0.01, 1000000);
    priceSpin->setDecimals(2);
    priceSpin->setPrefix(Currency::displaySymbol(CurrencyProvider::code()));
    auto *noteEdit = new QLineEdit(&dlg);
    form->addRow(QStringLiteral("物品名（market hash name）"), nameEdit);
    form->addRow(QStringLiteral("数量"), qtySpin);
    form->addRow(QStringLiteral("单价"), priceSpin);
    form->addRow(QStringLiteral("备注"), noteEdit);
    auto *feeHint = new QLabel(QStringLiteral("手续费按当前费率自动计算（买入计入成本，卖出从所得扣除）"), &dlg);
    feeHint->setWordWrap(true);
    form->addRow(feeHint);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;

    const QString error = m_trades->record(
        nameEdit->text().trimmed(),
        buy ? TradeRecord::Side::kBuy : TradeRecord::Side::kSell, qtySpin->value(),
        priceSpin->value(), noteEdit->text().trimmed());
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("记录失败"), error);
    } else {
        QMessageBox::information(this, QStringLiteral("已记录"),
                                 QStringLiteral("模拟交易已记录（含手续费）"));
    }
}

void PortfolioPage::showTrades() {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("模拟交易记录"));
    dlg.resize(760, 420);
    auto *table = new QTableWidget(0, 7, &dlg);
    table->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("物品"),
                                      QStringLiteral("方向"), QStringLiteral("数量"),
                                      QStringLiteral("单价"), QStringLiteral("手续费"),
                                      QStringLiteral("金额")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    const QVector<TradeRecord> records = m_trades->records();
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    table->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const TradeRecord &r = records.at(i);
        table->setItem(i, 0,
                       new QTableWidgetItem(r.tradedAt.toLocalTime().toString(
                           QStringLiteral("yyyy-MM-dd HH:mm"))));
        table->setItem(i, 1, new QTableWidgetItem(r.marketHashName));
        table->setItem(i, 2,
                       new QTableWidgetItem(r.side == TradeRecord::Side::kBuy ? QStringLiteral("买入")
                                                                              : QStringLiteral("卖出")));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(r.quantity)));
        table->setItem(i, 4, new QTableWidgetItem(symbol + QString::number(r.price, 'f', 2)));
        table->setItem(i, 5, new QTableWidgetItem(symbol + QString::number(r.fee, 'f', 2)));
        table->setItem(i, 6, new QTableWidgetItem(symbol + QString::number(r.total, 'f', 2)));
    }
    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(table);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    dlg.exec();
}

void PortfolioPage::reload() {
    m_model->removeRows(0, m_model->rowCount());
    const QVector<PortfolioItem> items = m_service->items();
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    for (const PortfolioItem &it : items) {
        auto *nameItem = new QStandardItem(it.marketHashName);
        nameItem->setData(it.id, Qt::UserRole);
        nameItem->setData(it.marketHashName, Qt::UserRole + 1);
        nameItem->setData(it.appid, Qt::UserRole + 2);
        const QString cost = it.purchasePrice > 0
                                 ? symbol + QString::number(it.purchasePrice, 'f', 2)
                                 : QStringLiteral("—");
        const QString latest = it.hasPrice
                                   ? symbol + QString::number(it.latestPrice, 'f', 2)
                                   : QStringLiteral("—");
        const QString value = it.hasPrice
                                  ? symbol + QString::number(it.marketValue, 'f', 2)
                                  : QStringLiteral("—");
        const QString pnl = it.hasPrice && it.purchasePrice > 0
                                ? symbol + QString::number(it.profitLoss, 'f', 2)
                                : QStringLiteral("—");
        const QString pnlPct = it.hasPrice && it.purchasePrice > 0
                                   ? QStringLiteral("%1%2%")
                                         .arg(it.profitLossPercent >= 0 ? QStringLiteral("+") : QString())
                                         .arg(it.profitLossPercent, 0, 'f', 2)
                                   : QStringLiteral("—");
        auto *pnlItem = new QStandardItem(pnl);
        if (it.hasPrice && it.purchasePrice > 0) {
            pnlItem->setForeground(QColor(it.profitLoss >= 0 ? ThemeProvider::upColor()
                                                             : ThemeProvider::downColor()));
        }
        m_model->appendRow({nameItem, new QStandardItem(QString::number(it.quantity)),
                            new QStandardItem(cost), new QStandardItem(latest),
                            new QStandardItem(value), pnlItem, new QStandardItem(pnlPct)});
    }
    const PortfolioSummary s = m_service->summary();
    m_loading->hide();
    m_summary->setText(QStringLiteral("总市值 %5%1  ·  总成本 %5%2  ·  盈亏 %5%3 (%4%)")
                           .arg(s.totalMarketValue, 0, 'f', 2)
                           .arg(s.totalCost, 0, 'f', 2)
                           .arg(s.totalProfitLoss, 0, 'f', 2)
                           .arg(s.totalProfitLossPercent, 0, 'f', 1)
                           .arg(symbol));
    m_status->setText(s.missingPriceCount > 0
                          ? QStringLiteral("有 %1 项缺少市价数据，已从汇总中排除")
                                .arg(s.missingPriceCount)
                          : QStringLiteral("共 %1 项").arg(s.itemCount));
    updateEmptyState(s.itemCount == 0);
}

void PortfolioPage::addEdit(int editId) {
    PortfolioItem current;
    if (editId > 0) {
        for (const PortfolioItem &it : m_service->items()) {
            if (it.id == editId) current = it;
        }
    }
    QDialog dlg(this);
    dlg.setWindowTitle(editId > 0 ? QStringLiteral("编辑持仓") : QStringLiteral("添加持仓"));
    auto *form = new QFormLayout(&dlg);
    auto *nameEdit = new QLineEdit(current.marketHashName, &dlg);
    auto *qtySpin = new QSpinBox(&dlg);
    qtySpin->setRange(1, 100000);
    qtySpin->setValue(current.quantity);
    auto *costSpin = new QDoubleSpinBox(&dlg);
    costSpin->setRange(0, 1000000);
    costSpin->setDecimals(2);
    costSpin->setPrefix(Currency::displaySymbol(CurrencyProvider::code()));
    costSpin->setValue(current.purchasePrice > 0 ? current.purchasePrice : 0);
    auto *noteEdit = new QLineEdit(current.note, &dlg);
    form->addRow(QStringLiteral("物品名（market hash name）"), nameEdit);
    form->addRow(QStringLiteral("数量"), qtySpin);
    form->addRow(QStringLiteral("成本单价"), costSpin);
    form->addRow(QStringLiteral("备注"), noteEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;
    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("物品名称不能为空"));
        return;
    }
    PortfolioItem next = current;
    next.marketHashName = nameEdit->text().trimmed();
    next.quantity = qtySpin->value();
    next.purchasePrice = costSpin->value() > 0 ? costSpin->value() : -1.0;
    next.note = noteEdit->text().trimmed();
    const bool ok = editId > 0 ? m_service->update(next) : m_service->add(next);
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QStringLiteral("物品不存在或写入失败，请先在行情页搜索该物品"));
    }
}
