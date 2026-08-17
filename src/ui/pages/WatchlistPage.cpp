#include "ui/pages/WatchlistPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "core/services/AlertService.h"
#include "core/services/WatchlistService.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"
#include "utils/ThemeProvider.h"

WatchlistPage::WatchlistPage(WatchlistService *watchlist, AlertService *alerts, QWidget *parent)
    : QWidget(parent), m_watchlist(watchlist), m_alerts(alerts) {
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    auto *removeBtn = new QPushButton(QStringLiteral("移除选中"), this);
    auto *alertBtn = new QPushButton(QStringLiteral("设置提醒"), this);
    auto *topRow = new QHBoxLayout();
    topRow->addWidget(refreshBtn);
    topRow->addWidget(removeBtn);
    topRow->addWidget(alertBtn);
    topRow->addStretch();

    m_table = new QTableView(this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(
        {QStringLiteral("物品名称"), QStringLiteral("最新价"), QStringLiteral("24h 涨跌"),
         QStringLiteral("销量"), QStringLiteral("备注")});
    m_table->setModel(m_model);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_status = new QLabel(this);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_status);

    connect(refreshBtn, &QPushButton::clicked, m_watchlist, &WatchlistService::refreshAll);
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        const QModelIndex idx = m_table->currentIndex();
        if (!idx.isValid()) return;
        const QString hash = m_model->item(idx.row(), 0)->data(Qt::UserRole).toString();
        if (QMessageBox::question(this, QStringLiteral("确认"),
                                  QStringLiteral("从自选移除 %1？").arg(hash))
            == QMessageBox::Yes) {
            m_watchlist->remove(hash);
        }
    });
    connect(alertBtn, &QPushButton::clicked, this, &WatchlistPage::setupAlertForSelected);
    connect(m_table, &QTableView::doubleClicked, this, [this]() {
        const QModelIndex idx = m_table->currentIndex();
        if (idx.isValid()) {
            emit itemSelected(m_model->item(idx.row(), 0)->data(Qt::UserRole).toString(),
                              m_model->item(idx.row(), 0)->data(Qt::UserRole + 1).toInt());
        }
    });
    connect(m_watchlist, &WatchlistService::listChanged, this, &WatchlistPage::reload);
    connect(m_watchlist, &WatchlistService::refreshFinished, this,
            [this](const QString &error) {
        m_status->setText(error.isEmpty() ? QStringLiteral("已刷新")
                                          : QStringLiteral("部分刷新失败：%1").arg(error));
    });
}

void WatchlistPage::reload() {
    m_model->removeRows(0, m_model->rowCount());
    const QVector<WatchlistItem> items = m_watchlist->items();
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    for (const WatchlistItem &it : items) {
        auto *nameItem = new QStandardItem(it.marketHashName);
        nameItem->setData(it.marketHashName, Qt::UserRole);
        nameItem->setData(it.appid, Qt::UserRole + 1);
        const QString price = it.hasPrice
                                  ? symbol + QString::number(it.latestPrice, 'f', 2)
                                  : QStringLiteral("—");
        const QString change = it.hasPrice
                                   ? QStringLiteral("%1%2%")
                                         .arg(it.changePercent24h >= 0 ? QStringLiteral("+") : QString())
                                         .arg(it.changePercent24h, 0, 'f', 2)
                                   : QStringLiteral("—");
        const QString volume = it.volume >= 0 ? QString::number(it.volume) : QStringLiteral("—");
        auto *changeItem = new QStandardItem(change);
        if (it.hasPrice) {
            const bool up = it.changePercent24h >= 0;
            changeItem->setForeground(QColor(up ? ThemeProvider::upColor()
                                                : ThemeProvider::downColor()));
        }
        m_model->appendRow({nameItem, new QStandardItem(price), changeItem,
                            new QStandardItem(volume), new QStandardItem(it.note)});
    }
    m_status->setText(items.isEmpty() ? QStringLiteral("自选为空：在详情页点击“加入自选”")
                                      : QStringLiteral("共 %1 项").arg(items.size()));
}

void WatchlistPage::setupAlertForSelected() {
    const QModelIndex idx = m_table->currentIndex();
    if (!idx.isValid()) return;
    const QString hash = m_model->item(idx.row(), 0)->data(Qt::UserRole).toString();

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("设置提醒：%1").arg(hash));
    auto *form = new QFormLayout(&dlg);
    auto *typeBox = new QComboBox(&dlg);
    typeBox->addItems({QStringLiteral("低于阈值"), QStringLiteral("高于阈值"),
                       QStringLiteral("24h 涨跌幅超过")});
    auto *valueEdit = new QLineEdit(&dlg);
    form->addRow(QStringLiteral("条件"), typeBox);
    form->addRow(QStringLiteral("阈值"), valueEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;

    bool ok = false;
    const double value = valueEdit->text().toDouble(&ok);
    if (!ok || value <= 0) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请输入大于 0 的数值"));
        return;
    }
    Alert alert;
    alert.marketHashName = hash;
    alert.appid = 730;
    if (typeBox->currentIndex() == 2) {
        alert.conditionType = Alert::Condition::kPercent24h;
        alert.percentValue = value;
    } else {
        alert.conditionType = typeBox->currentIndex() == 1 ? Alert::Condition::kAbove
                                                           : Alert::Condition::kBelow;
        alert.thresholdValue = value;
    }
    m_alerts->add(alert);
    QMessageBox::information(this, QStringLiteral("提醒已创建"), QStringLiteral("已创建价格提醒"));
}
