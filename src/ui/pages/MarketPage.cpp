#include "ui/pages/MarketPage.h"

#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "core/services/MarketService.h"
#include "ui/widgets/IconCache.h"
#include "ui/widgets/LoadingOverlay.h"
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

    m_moreBtn = new QPushButton(QStringLiteral("加载更多"), this);
    m_moreBtn->setVisible(false);
    m_moreBtn->setToolTip(QStringLiteral("搜索默认每页 30 条，点击加载下一页结果"));
    auto *moreRow = new QHBoxLayout();
    moreRow->addStretch(1);
    moreRow->addWidget(m_moreBtn);
    moreRow->addStretch(1);

    m_card = new QFrame(this);
    m_card->setObjectName(QStringLiteral("pageCard"));
    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);
    cardLayout->addLayout(topRow);
    cardLayout->addWidget(m_table, 1);
    cardLayout->addLayout(moreRow);
    cardLayout->addWidget(m_status);

    m_emptyState = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(8);
    m_emptyIcon = new QLabel(QStringLiteral("🔍"), m_emptyState);
    m_emptyIcon->setObjectName(QStringLiteral("emptyStateIcon"));
    m_emptyIcon->setAlignment(Qt::AlignCenter);
    m_emptyTitle = new QLabel(QStringLiteral("输入关键词开始搜索"), m_emptyState);
    m_emptyTitle->setObjectName(QStringLiteral("emptyStateTitle"));
    m_emptyTitle->setAlignment(Qt::AlignCenter);
    m_emptySubtitle = new QLabel(QStringLiteral("试试输入 AK-47、Asiimov 等物品名称"), m_emptyState);
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
    m_loading->setText(QStringLiteral("正在搜索 Steam 市场…"));
    connect(m_searchButton, &QPushButton::clicked, this, &MarketPage::runSearch);
    connect(m_search, &QLineEdit::returnPressed, this, &MarketPage::runSearch);
    connect(m_game, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { runSearch(); });
    connect(m_table, &QTableView::activated, this, &MarketPage::openCurrentItem);
    connect(m_moreBtn, &QPushButton::clicked, this, &MarketPage::loadMore);
    connect(IconCache::instance(), &IconCache::ready, this, &MarketPage::onIconReady);

    connect(m_service, &MarketService::searchFinished, this, &MarketPage::render);
}

void MarketPage::runSearch() {
    if (!m_searchButton->isEnabled()) return;
    m_query = m_search->text().trimmed();
    m_appid = m_game->currentData().toInt();
    m_total = 0;
    m_appending = false;
    m_moreBtn->setVisible(false);
    m_searchButton->setEnabled(false);
    m_searchButton->setText(QStringLiteral("搜索中…"));
    m_table->setEnabled(false);
    m_status->setProperty("error", false);
    m_status->setText(QStringLiteral("搜索中…"));
    m_loading->setText(QStringLiteral("正在搜索 Steam 市场…"));
    m_loading->show();
    m_service->search(m_query, m_appid, 0);
}

void MarketPage::loadMore() {
    if (m_query.isEmpty() || m_appending) return;
    m_appending = true;
    m_moreBtn->setEnabled(false);
    m_moreBtn->setText(QStringLiteral("加载中…"));
    m_loading->setText(QStringLiteral("正在加载更多结果…"));
    m_loading->show();
    m_service->search(m_query, m_appid, m_model->rowCount());
}

void MarketPage::setGame(int appid) {
    const int idx = m_game->findData(appid);
    if (idx >= 0) {
        m_game->setCurrentIndex(idx);
    }
}

void MarketPage::render(const QVector<MarketItem> &items, int totalCount, const QString &error) {
    const bool append = m_appending;
    m_appending = false;
    m_loading->hide();
    m_searchButton->setEnabled(true);
    m_searchButton->setText(QStringLiteral("搜索行情"));
    m_table->setEnabled(true);
    m_status->setProperty("error", !error.isEmpty());
    m_status->setStyleSheet(error.isEmpty() ? QString() : kErrorStyle);
    if (!error.isEmpty()) {
        m_status->setText(QStringLiteral("搜索失败：%1").arg(error));
        updateMoreButton();
        updateEmptyState(false);
        return;
    }
    if (!append) m_model->removeRows(0, m_model->rowCount());
    if (items.isEmpty() && !append) {
        m_status->setText(QStringLiteral("无匹配物品，请尝试其他关键词"));
        m_total = 0;
        updateMoreButton();
        updateEmptyState(true);
        return;
    }
    appendItems(items);
    if (totalCount >= 0) m_total = totalCount;
    updateMoreButton();
    m_status->setText(QStringLiteral("已展示 %1 / 共 %2 条  ·  双击或按 Enter 查看详情")
                          .arg(m_model->rowCount())
                          .arg(m_total > 0 ? m_total : m_model->rowCount()));
    if (!append) m_table->selectRow(0);
    updateEmptyState(false);
}

void MarketPage::updateEmptyState(bool empty) {
    auto *stack = qobject_cast<QStackedWidget *>(layout()->itemAt(0)->widget());
    if (stack) stack->setCurrentIndex(empty ? 1 : 0);
}

void MarketPage::appendItems(const QVector<MarketItem> &items) {
    // 分页追加：已存在的 market_hash_name 不重复添加。
    QSet<QString> existing;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        existing.insert(m_model->item(row, 0)->data(Qt::UserRole).toString());
    }
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    for (const MarketItem &item : items) {
        if (existing.contains(item.marketHashName)) continue;
        existing.insert(item.marketHashName);
        auto *nameItem = new QStandardItem(item.name);
        nameItem->setData(item.marketHashName, Qt::UserRole);
        nameItem->setData(item.appid, Qt::UserRole + 1);
        nameItem->setData(item.iconUrl, Qt::UserRole + 2);
        nameItem->setToolTip(item.marketHashName);
        const QPixmap pix = IconCache::instance()->pixmap(item.iconUrl);
        if (!pix.isNull()) nameItem->setIcon(QIcon(pix));
        auto *priceItem = new QStandardItem(item.hasPrice
                                                ? symbol + QString::number(item.price, 'f', 2)
                                                : QStringLiteral("—"));
        priceItem->setData(item.price, Qt::UserRole);
        auto *volItem = new QStandardItem(item.volume > 0 ? QString::number(item.volume)
                                                          : QStringLiteral("—"));
        m_model->appendRow({nameItem, priceItem, volItem});
    }
}

void MarketPage::updateMoreButton() {
    const bool hasMore = m_total > m_model->rowCount();
    m_moreBtn->setVisible(hasMore);
    m_moreBtn->setEnabled(true);
    m_moreBtn->setText(QStringLiteral("加载更多（剩余 %1 条）")
                           .arg(m_total - m_model->rowCount()));
}

void MarketPage::onIconReady(const QString &iconPath, const QPixmap &pixmap) {
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStandardItem *nameItem = m_model->item(row, 0);
        if (nameItem && nameItem->data(Qt::UserRole + 2).toString() == iconPath) {
            nameItem->setIcon(QIcon(pixmap));
        }
    }
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

