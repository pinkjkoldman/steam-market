#include "ui/pages/MarketPage.h"

#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "core/services/MarketService.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"

namespace {
// 错误态与中性状态样式分离，避免普通提示也套用红色错误外观。
const QString kErrorStyle = QStringLiteral(
    "background:#412528;color:#FFB8BE;border:1px solid #71343A;border-radius:6px;padding:8px 10px;");
}  // namespace

MarketPage::MarketPage(MarketService *service, QWidget *parent)
    : QWidget(parent), m_service(service) {
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(QStringLiteral("搜索 Steam 市场物品，如：AK-47 | Redline"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(320);
    m_search->setToolTip(QStringLiteral("输入物品中文名或英文市场名称，按 Enter 搜索"));
    m_game = new QComboBox(this);
    m_game->addItem(QStringLiteral("CS2"), 730);
    m_game->addItem(QStringLiteral("DOTA2"), 570);
    m_game->addItem(QStringLiteral("军团要塞2（预留）"), 440);
    // TF2 暂不启用（用户要求先不加入）
    if (auto *model = qobject_cast<QStandardItemModel *>(m_game->model())) {
        if (QStandardItem *tf2Item = model->item(2)) {
            tf2Item->setEnabled(false);
        }
    }
    m_searchButton = new QPushButton(QStringLiteral("搜索行情"), this);
    m_searchButton->setObjectName(QStringLiteral("primaryButton"));
    m_searchButton->setDefault(true);
    auto *topRow = new QHBoxLayout();
    topRow->addWidget(m_search, 1);
    topRow->addWidget(m_game);
    topRow->addWidget(m_searchButton);

    m_table = new QTableView(this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSortingEnabled(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({QStringLiteral("物品名称"), QStringLiteral("当前价"),
                                        QStringLiteral("销量")});
    m_table->setModel(m_model);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_status = new QLabel(QStringLiteral("输入关键词开始搜索，也可直接按 Enter"), this);
    m_status->setObjectName(QStringLiteral("inlineStatus"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_status);

    connect(m_searchButton, &QPushButton::clicked, this, &MarketPage::runSearch);
    connect(m_search, &QLineEdit::returnPressed, this, &MarketPage::runSearch);
    connect(m_game, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { runSearch(); });
    connect(m_table, &QTableView::activated, this, &MarketPage::openCurrentItem);

    connect(m_service, &MarketService::searchFinished, this, &MarketPage::render);
}

void MarketPage::runSearch() {
    if (!m_searchButton->isEnabled()) return;
    m_searchButton->setEnabled(false);
    m_searchButton->setText(QStringLiteral("搜索中…"));
    m_table->setEnabled(false);
    m_status->setProperty("error", false);
    m_status->setText(QStringLiteral("搜索中…"));
    m_service->search(m_search->text(), m_game->currentData().toInt());
}

void MarketPage::setGame(int appid) {
    const int idx = m_game->findData(appid);
    if (idx >= 0) {
        m_game->setCurrentIndex(idx);
    }
}

void MarketPage::render(const QVector<MarketItem> &items, const QString &error) {
    m_model->removeRows(0, m_model->rowCount());
    m_searchButton->setEnabled(true);
    m_searchButton->setText(QStringLiteral("搜索行情"));
    m_table->setEnabled(true);
    m_status->setProperty("error", !error.isEmpty());
    m_status->setStyleSheet(error.isEmpty() ? QString() : kErrorStyle);
    if (!error.isEmpty()) {
        m_status->setText(QStringLiteral("搜索失败：%1").arg(error));
        return;
    }
    if (items.isEmpty()) {
        m_status->setText(QStringLiteral("无匹配物品，请尝试其他关键词"));
        return;
    }
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    for (const MarketItem &item : items) {
        auto *nameItem = new QStandardItem(item.name);
        nameItem->setData(item.marketHashName, Qt::UserRole);
        nameItem->setData(item.appid, Qt::UserRole + 1);
        nameItem->setToolTip(item.marketHashName);
        auto *priceItem = new QStandardItem(item.hasPrice
                                                ? symbol + QString::number(item.price, 'f', 2)
                                                : QStringLiteral("—"));
        priceItem->setData(item.price, Qt::UserRole);
        auto *volItem = new QStandardItem(item.volume > 0 ? QString::number(item.volume)
                                                          : QStringLiteral("—"));
        m_model->appendRow({nameItem, priceItem, volItem});
    }
    m_status->setText(QStringLiteral("共 %1 条结果  ·  双击或按 Enter 查看详情").arg(items.size()));
    m_table->selectRow(0);
}
void MarketPage::openCurrentItem() {
    const QModelIndex idx = m_table->currentIndex();
    if (!idx.isValid()) return;
    const QString hash = m_model->item(idx.row(), 0)->data(Qt::UserRole).toString();
    const QString name = m_model->item(idx.row(), 0)->text();
    const int appid = m_model->item(idx.row(), 0)->data(Qt::UserRole + 1).toInt();
    MarketItem item;
    item.marketHashName = hash;
    item.name = name;
    item.appid = appid > 0 ? appid : m_game->currentData().toInt();
    const auto *priceItem = m_model->item(idx.row(), 1);
    item.hasPrice = priceItem && priceItem->data(Qt::UserRole).toDouble() > 0;
    item.price = priceItem ? priceItem->data(Qt::UserRole).toDouble() : 0.0;
    emit itemSelected(item);
}

