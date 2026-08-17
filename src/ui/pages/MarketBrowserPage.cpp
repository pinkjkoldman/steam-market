#include "ui/pages/MarketBrowserPage.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ui/widgets/QuickInspector.h"
#include "ui/widgets/ScopeNotice.h"
#include "ui/widgets/WorkbenchTheme.h"
#include "utils/Currency.h"

namespace {

QString gameName(int appid) {
  switch (appid) {
    case 730: return QStringLiteral("CS2");
    case 570: return QStringLiteral("Dota 2");
    case 440: return QStringLiteral("TF2");
    case 753: return QStringLiteral("Steam 社区");
    default: return QStringLiteral("App %1").arg(appid);
  }
}

QString priceText(qint64 minor, const QString &currency) {
  return Currency::displaySymbol(currency)
         + QLocale().toString(static_cast<double>(minor) / 100.0, 'f', 2);
}

}  // namespace

MarketBrowserPage::MarketBrowserPage(QWidget *parent) : QWidget(parent) {
  setProperty("workbench", true);
  setStyleSheet(WorkbenchTheme::styleSheet());
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(32, 24, 32, 24);
  root->setSpacing(14);

  auto *heading = new QHBoxLayout();
  auto *titles = new QVBoxLayout();
  auto *title = new QLabel(QStringLiteral("物品浏览"), this);
  title->setObjectName(QStringLiteral("pageTitle"));
  auto *subtitle = new QLabel(QStringLiteral("Steam 全市场远端搜索与按需分页"), this);
  subtitle->setObjectName(QStringLiteral("pageSubtitle"));
  titles->addWidget(title);
  titles->addWidget(subtitle);
  heading->addLayout(titles, 1);
  m_search = new QLineEdit(this);
  m_search->setPlaceholderText(QStringLiteral("搜索名称或 market_hash_name（按 Enter 立即搜索）"));
  m_search->setClearButtonEnabled(true);
  m_search->setMinimumWidth(380);
  heading->addWidget(m_search);
  auto *searchButton = new QPushButton(QStringLiteral("搜索"), this);
  searchButton->setObjectName(QStringLiteral("primaryButton"));
  heading->addWidget(searchButton);
  root->addLayout(heading);

  auto *filters = new QHBoxLayout();
  m_game = new QComboBox(this);
  m_game->addItem(QStringLiteral("全部游戏"), 0);
  m_game->addItem(QStringLiteral("Counter-Strike 2"), 730);
  m_game->addItem(QStringLiteral("Dota 2"), 570);
  m_game->addItem(QStringLiteral("Team Fortress 2"), 440);
  m_game->addItem(QStringLiteral("Steam 社区物品"), 753);
  m_sort = new QComboBox(this);
  m_sort->addItem(QStringLiteral("热门排序"), QStringLiteral("popular"));
  m_sort->addItem(QStringLiteral("价格从低到高"), QStringLiteral("price_asc"));
  m_sort->addItem(QStringLiteral("价格从高到低"), QStringLiteral("price_desc"));
  m_sort->addItem(QStringLiteral("名称排序"), QStringLiteral("name_asc"));
  m_clear = new QPushButton(QStringLiteral("清除筛选"), this);
  m_clear->setObjectName(QStringLiteral("linkButton"));
  filters->addWidget(new QLabel(QStringLiteral("游戏"), this));
  filters->addWidget(m_game);
  filters->addWidget(new QLabel(QStringLiteral("排序"), this));
  filters->addWidget(m_sort);
  filters->addWidget(m_clear);
  filters->addStretch();
  m_filterHint = new QLabel(QStringLiteral("所有筛选均映射至 Steam 远端查询"), this);
  m_filterHint->setObjectName(QStringLiteral("mutedText"));
  filters->addWidget(m_filterHint);
  root->addLayout(filters);

  auto *content = new QGridLayout();
  content->setHorizontalSpacing(16);
  m_table = new QTableView(this);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setAlternatingRowColors(true);
  m_table->setSortingEnabled(false);
  m_table->verticalHeader()->setVisible(false);
  m_table->verticalHeader()->setDefaultSectionSize(46);
  m_model = new QStandardItemModel(this);
  m_model->setHorizontalHeaderLabels({QStringLiteral("物品"), QStringLiteral("游戏 / 类型"),
                                      QStringLiteral("最低挂单价"), QStringLiteral("挂单量")});
  m_table->setModel(m_model);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  content->addWidget(m_table, 0, 0, 1, 8);
  m_inspector = new QuickInspector(this);
  content->addWidget(m_inspector, 0, 8, 1, 4);
  content->setColumnStretch(0, 1);
  root->addLayout(content, 1);

  auto *pager = new QHBoxLayout();
  m_range = new QLabel(QStringLiteral("第 0–0 项 / 共 0 项"), this);
  m_range->setObjectName(QStringLiteral("mutedText"));
  m_previous = new QPushButton(QStringLiteral("上一页"), this);
  m_next = new QPushButton(QStringLiteral("下一页"), this);
  pager->addWidget(m_range);
  pager->addStretch();
  pager->addWidget(m_previous);
  pager->addWidget(m_next);
  root->addLayout(pager);
  m_notice = new ScopeNotice(this);
  root->addWidget(m_notice);

  m_debounce = new QTimer(this);
  m_debounce->setSingleShot(true);
  m_debounce->setInterval(300);
  connect(m_search, &QLineEdit::textChanged, m_debounce, qOverload<>(&QTimer::start));
  connect(m_debounce, &QTimer::timeout, this, [this]() { requestPage(0); });
  connect(m_search, &QLineEdit::returnPressed, this, [this]() {
    m_debounce->stop();
    requestPage(0);
  });
  connect(searchButton, &QPushButton::clicked, this, [this]() {
    m_debounce->stop();
    requestPage(0);
  });
  connect(m_game, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
    m_filterHint->setText(QStringLiteral("已切换游戏范围；不兼容筛选已清除"));
    requestPage(0);
  });
  connect(m_sort, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int) { requestPage(0); });
  connect(m_clear, &QPushButton::clicked, this, &MarketBrowserPage::clearFilters);
  connect(m_previous, &QPushButton::clicked, this,
          [this]() { requestPage(qMax(0, m_offset - m_pageSize)); });
  connect(m_next, &QPushButton::clicked, this,
          [this]() { requestPage(m_offset + m_pageSize); });
  connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          [this]() { selectCurrentRow(); });
  connect(m_table, &QTableView::activated, this,
          [this]() { activateCurrentRow(); });
  connect(m_inspector, &QuickInspector::analysisRequested, this,
          &MarketBrowserPage::itemActivated);
  connect(m_inspector, &QuickInspector::followRequested, this,
          &MarketBrowserPage::followRequested);
  connect(m_inspector, &QuickInspector::alertRequested, this,
          &MarketBrowserPage::alertRequested);
  connect(m_notice, &ScopeNotice::retryRequested, this, &MarketBrowserPage::retryRequested);
  connect(m_notice, &ScopeNotice::openOfficialMarketRequested, this,
          &MarketBrowserPage::openOfficialMarketRequested);
  updatePagination();
}

void MarketBrowserPage::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  if (!m_requestedOnce) {
    m_requestedOnce = true;
    requestPage(m_offset);
  }
}

void MarketBrowserPage::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Slash && !m_search->hasFocus()) {
    focusSearch();
    event->accept();
    return;
  }
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    activateCurrentRow();
    event->accept();
    return;
  }
  if (event->key() == Qt::Key_Escape && m_inspector->isVisible()) {
    m_table->clearSelection();
    m_inspector->clearItem();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

void MarketBrowserPage::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (event->size().width() < 1040) {
    m_table->horizontalHeader()->hideSection(1);
    m_inspector->setMaximumHeight(260);
  } else {
    m_table->horizontalHeader()->showSection(1);
    m_inspector->setMaximumHeight(QWIDGETSIZE_MAX);
  }
}

void MarketBrowserPage::setInitialQuery(const QString &query, int appid) {
  m_search->setText(query);
  const int index = m_game->findData(appid);
  m_game->setCurrentIndex(index >= 0 ? index : 0);
  m_offset = 0;
}

void MarketBrowserPage::setPage(const MarketCatalogPageView &page) {
  m_offset = page.offset;
  m_pageSize = page.pageSize > 0 ? page.pageSize : 10;
  m_totalCount = page.totalCount;
  m_model->removeRows(0, m_model->rowCount());
  for (const auto &item : page.items) {
    auto *name = new QStandardItem(item.name);
    name->setData(item.appid, Qt::UserRole);
    name->setData(item.marketHashName, Qt::UserRole + 1);
    name->setData(QVariant::fromValue(item), Qt::UserRole + 2);
    name->setToolTip(item.marketHashName);
    const QString identity = item.typeText.isEmpty()
                                 ? gameName(item.appid)
                                 : QStringLiteral("%1 · %2").arg(gameName(item.appid), item.typeText);
    auto *price = new QStandardItem(priceText(item.lowestSellMinor, m_currency));
    price->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *listings = new QStandardItem(QLocale().toString(item.sellListings));
    listings->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_model->appendRow({name, new QStandardItem(identity), price, listings});
  }
  if (m_model->rowCount() > 0) m_table->selectRow(0);
  else m_inspector->clearItem();
  updatePagination();
  m_notice->setPageContext(page, scopeLabel(), m_currency);
}

void MarketBrowserPage::setStatus(const MarketUiStatus &status) {
  m_notice->setStatus(status);
  const bool busy = status.state == MarketViewState::Loading;
  m_previous->setEnabled(!busy && m_offset > 0);
  m_next->setEnabled(!busy && m_offset + m_pageSize < m_totalCount);
  if (status.state == MarketViewState::Empty) {
    m_range->setText(QStringLiteral("当前筛选没有可检索物品 · 可清除筛选重试"));
  }
}

void MarketBrowserPage::setInspection(const MarketInspectionView &inspection) {
  m_inspector->setInspection(inspection);
}

void MarketBrowserPage::focusSearch() {
  m_search->setFocus(Qt::ShortcutFocusReason);
  m_search->selectAll();
}

void MarketBrowserPage::setCurrency(const QString &code) {
  const QString next = code.trimmed().isEmpty() ? QStringLiteral("CNY") : code.trimmed();
  if (next == m_currency) return;
  m_currency = next;
  if (m_requestedOnce) requestPage(m_offset);
}

void MarketBrowserPage::requestPage(int offset) {
  m_offset = qMax(0, offset);
  emit catalogRequested(m_search->text().trimmed(), m_game->currentData().toInt(), m_offset, 10,
                        m_sort->currentData().toString(), m_currency);
}

void MarketBrowserPage::selectCurrentRow() {
  const QModelIndex index = m_table->currentIndex();
  if (!index.isValid()) return;
  const auto item = m_model->item(index.row(), 0)
                        ->data(Qt::UserRole + 2).value<MarketCatalogItemView>();
  m_inspector->setItem(item, m_currency);
  emit itemInspectRequested(item.appid, item.marketHashName);
}

void MarketBrowserPage::activateCurrentRow() {
  const QModelIndex index = m_table->currentIndex();
  if (!index.isValid()) return;
  emit itemActivated(m_model->item(index.row(), 0)->data(Qt::UserRole).toInt(),
                     m_model->item(index.row(), 0)->data(Qt::UserRole + 1).toString());
}

void MarketBrowserPage::clearFilters() {
  m_debounce->stop();
  m_search->clear();
  m_game->setCurrentIndex(0);
  m_sort->setCurrentIndex(0);
  m_filterHint->setText(QStringLiteral("所有筛选均映射至 Steam 远端查询"));
  requestPage(0);
}

void MarketBrowserPage::updatePagination() {
  const qint64 first = m_model->rowCount() > 0 ? m_offset + 1 : 0;
  const qint64 last = m_model->rowCount() > 0 ? m_offset + m_model->rowCount() : 0;
  m_range->setText(QStringLiteral("第 %1–%2 项 / 共 %3 项")
                       .arg(QLocale().toString(first), QLocale().toString(last),
                            QLocale().toString(m_totalCount)));
  m_previous->setEnabled(m_offset > 0);
  m_next->setEnabled(m_offset + m_pageSize < m_totalCount);
}

QString MarketBrowserPage::scopeLabel() const {
  return m_game->currentData().toInt() == 0 ? QStringLiteral("全站") : m_game->currentText();
}

