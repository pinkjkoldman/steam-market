#include "ui/pages/MarketBrowserPage.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
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
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ui/widgets/QuickInspector.h"
#include "ui/widgets/MarketPageFilterEngine.h"
#include "ui/widgets/ScopeNotice.h"
#include "ui/widgets/LoadingOverlay.h"
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
  setMinimumHeight(660);
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
  root->addLayout(heading);

  auto *searchRow = new QHBoxLayout();
  m_search = new QLineEdit(this);
  m_search->setPlaceholderText(QStringLiteral("搜索名称或 market_hash_name（按 Enter 立即搜索）"));
  m_search->setClearButtonEnabled(true);
  m_search->setMinimumWidth(0);
  searchRow->addWidget(m_search, 1);
  auto *searchButton = new QPushButton(QStringLiteral("搜索"), this);
  searchButton->setObjectName(QStringLiteral("primaryButton"));
  searchRow->addWidget(searchButton);
  root->addLayout(searchRow);

  auto *filters = new QGridLayout();
  filters->setHorizontalSpacing(10);
  filters->setVerticalSpacing(8);
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
  m_preset = new QComboBox(this);
  m_preset->addItem(QStringLiteral("快速定位常用物品…"));
  m_preset->addItem(QStringLiteral("CS2 · 武器箱"), QStringLiteral("730|Case"));
  m_preset->addItem(QStringLiteral("CS2 · 印花"), QStringLiteral("730|Sticker"));
  m_preset->addItem(QStringLiteral("CS2 · 刀具"), QStringLiteral("730|Knife"));
  m_preset->addItem(QStringLiteral("Dota 2 · Immortal"), QStringLiteral("570|Immortal"));
  m_preset->addItem(QStringLiteral("Steam · 交易卡牌"), QStringLiteral("753|Trading Card"));
  m_typeFilter = new QLineEdit(this);
  m_typeFilter->setPlaceholderText(QStringLiteral("例如：武器箱、印花、卡牌"));
  m_typeFilter->setClearButtonEnabled(true);
  m_minPrice = new QDoubleSpinBox(this);
  m_maxPrice = new QDoubleSpinBox(this);
  for (QDoubleSpinBox *price : {m_minPrice, m_maxPrice}) {
    price->setRange(0.0, 1000000.0);
    price->setDecimals(2);
    price->setSpecialValueText(QStringLiteral("不限"));
    price->setPrefix(Currency::displaySymbol(m_currency));
  }
  m_minListings = new QSpinBox(this);
  m_minListings->setRange(0, 100000000);
  m_minListings->setSpecialValueText(QStringLiteral("不限"));
  m_pricedOnly = new QCheckBox(QStringLiteral("仅看有报价"), this);
  m_pricedOnly->setChecked(true);
  m_clear = new QPushButton(QStringLiteral("清除筛选"), this);
  m_clear->setObjectName(QStringLiteral("linkButton"));
  filters->addWidget(new QLabel(QStringLiteral("远端游戏"), this), 0, 0);
  filters->addWidget(m_game, 0, 1);
  filters->addWidget(new QLabel(QStringLiteral("远端排序"), this), 0, 2);
  filters->addWidget(m_sort, 0, 3);
  filters->addWidget(m_preset, 1, 0, 1, 3);
  filters->addWidget(m_clear, 1, 3);
  filters->addWidget(new QLabel(QStringLiteral("当前页类型包含"), this), 2, 0);
  filters->addWidget(m_typeFilter, 2, 1, 1, 2);
  filters->addWidget(m_pricedOnly, 2, 3);
  filters->addWidget(new QLabel(QStringLiteral("当前页价格区间"), this), 3, 0);
  auto *priceRange = new QHBoxLayout();
  priceRange->addWidget(m_minPrice, 1);
  priceRange->addWidget(new QLabel(QStringLiteral("至"), this));
  priceRange->addWidget(m_maxPrice, 1);
  filters->addLayout(priceRange, 3, 1, 1, 3);
  filters->addWidget(new QLabel(QStringLiteral("当前页挂单量不少于"), this), 4, 0);
  filters->addWidget(m_minListings, 4, 1);
  m_filterHint = new QLabel(
      QStringLiteral("关键词 / 游戏 / 排序作用于全市场；价格 / 类型 / 挂单量只二次筛选当前页"),
      this);
  m_filterHint->setObjectName(QStringLiteral("mutedText"));
  m_filterHint->setWordWrap(true);
  filters->addWidget(m_filterHint, 4, 2, 1, 2);
  filters->setColumnStretch(1, 1);
  filters->setColumnStretch(3, 1);
  root->addLayout(filters);

  auto *metrics = new QHBoxLayout();
  m_totalMetric = new QLabel(QStringLiteral("远端命中\n0"), this);
  m_visibleMetric = new QLabel(QStringLiteral("当前页显示\n0 / 0"), this);
  m_priceMetric = new QLabel(QStringLiteral("可见中位价\n—"), this);
  m_listingsMetric = new QLabel(QStringLiteral("可见挂单总量\n0"), this);
  for (QLabel *metric : {m_totalMetric, m_visibleMetric, m_priceMetric, m_listingsMetric}) {
    metric->setObjectName(QStringLiteral("marketMetricCard"));
    metric->setAlignment(Qt::AlignCenter);
    metric->setMinimumHeight(54);
    metrics->addWidget(metric, 1);
  }
  m_totalMetric->setToolTip(QStringLiteral("Steam 对当前远端关键词和游戏范围返回的总匹配数"));
  m_visibleMetric->setToolTip(QStringLiteral("当前页经过价格、类型和挂单量二次筛选后的数量"));
  m_priceMetric->setToolTip(QStringLiteral("当前可见且有报价物品的价格中位数"));
  m_listingsMetric->setToolTip(QStringLiteral("当前可见物品的挂单量合计，用于直观看流动性"));
  root->addLayout(metrics);

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
                                      QStringLiteral("最低挂单价"), QStringLiteral("挂单量"),
                                      QStringLiteral("流动性")});
  m_table->setModel(m_model);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
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

  m_loading = new LoadingOverlay(this);
  m_loading->setText(QStringLiteral("正在检索 Steam 市场…"));

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
    requestPage(0);
  });
  connect(m_sort, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int) { requestPage(0); });
  connect(m_preset, qOverload<int>(&QComboBox::activated), this,
          &MarketBrowserPage::applyPreset);
  connect(m_typeFilter, &QLineEdit::textChanged, this,
          &MarketBrowserPage::applyLocalFilters);
  connect(m_minPrice, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          &MarketBrowserPage::applyLocalFilters);
  connect(m_maxPrice, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          &MarketBrowserPage::applyLocalFilters);
  connect(m_minListings, qOverload<int>(&QSpinBox::valueChanged), this,
          &MarketBrowserPage::applyLocalFilters);
  connect(m_pricedOnly, &QCheckBox::toggled, this,
          &MarketBrowserPage::applyLocalFilters);
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
  m_filterHint->setVisible(event->size().width() >= 900);
  if (event->size().width() < 1040) {
    m_table->horizontalHeader()->hideSection(1);
    m_inspector->setMaximumHeight(300);
  } else {
    m_table->horizontalHeader()->showSection(1);
    m_inspector->setMaximumHeight(QWIDGETSIZE_MAX);
  }
}

void MarketBrowserPage::setInitialQuery(const QString &query, int appid) {
  m_debounce->stop();
  const QSignalBlocker searchBlocker(m_search);
  const QSignalBlocker gameBlocker(m_game);
  m_search->setText(query);
  const int index = m_game->findData(appid);
  m_game->setCurrentIndex(index >= 0 ? index : 0);
  m_offset = 0;
  if (m_requestedOnce) requestPage(0);
}

void MarketBrowserPage::setPage(const MarketCatalogPageView &page) {
  m_offset = page.offset;
  m_pageSize = page.pageSize > 0 ? page.pageSize : 10;
  m_totalCount = page.totalCount;
  m_sourceItems = page.items;
  applyLocalFilters();
  m_notice->setPageContext(page, scopeLabel(), m_currency);
}

void MarketBrowserPage::applyLocalFilters() {
  MarketPageFilter filter;
  filter.typeContains = m_typeFilter->text();
  filter.minimumPriceMinor = qRound64(m_minPrice->value() * 100.0);
  filter.maximumPriceMinor = qRound64(m_maxPrice->value() * 100.0);
  filter.minimumListings = m_minListings->value();
  filter.pricedOnly = m_pricedOnly->isChecked();
  const MarketPageFilterResult filtered = MarketPageFilterEngine::apply(m_sourceItems, filter);
  m_filteredCount = filtered.items.size();
  m_model->removeRows(0, m_model->rowCount());
  for (const auto &item : filtered.items) {
    auto *name = new QStandardItem(item.name);
    name->setData(item.appid, Qt::UserRole);
    name->setData(item.marketHashName, Qt::UserRole + 1);
    name->setData(QVariant::fromValue(item), Qt::UserRole + 2);
    name->setToolTip(item.marketHashName);
    const QString identity = item.typeText.isEmpty()
                                 ? gameName(item.appid)
                                 : QStringLiteral("%1 · %2").arg(gameName(item.appid), item.typeText);
    auto *price = new QStandardItem(item.lowestSellMinor > 0
                                        ? priceText(item.lowestSellMinor, m_currency)
                                        : QStringLiteral("暂无报价"));
    price->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *listings = new QStandardItem(QLocale().toString(item.sellListings));
    listings->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QString liquidityText;
    QColor liquidityColor;
    if (item.sellListings >= 1000) {
      liquidityText = QStringLiteral("高");
      liquidityColor = QColor(QStringLiteral("#66D9A8"));
    } else if (item.sellListings >= 100) {
      liquidityText = QStringLiteral("中");
      liquidityColor = QColor(QStringLiteral("#8CD2FF"));
    } else if (item.sellListings > 0) {
      liquidityText = QStringLiteral("低");
      liquidityColor = QColor(QStringLiteral("#F3B95F"));
    } else {
      liquidityText = QStringLiteral("无");
      liquidityColor = QColor(QStringLiteral("#718397"));
    }
    auto *liquidity = new QStandardItem(liquidityText);
    liquidity->setTextAlignment(Qt::AlignCenter);
    liquidity->setForeground(liquidityColor);
    liquidity->setToolTip(QStringLiteral("按当前挂单量粗略分级：高 ≥1000，中 ≥100，低 <100"));
    m_model->appendRow({name, new QStandardItem(identity), price, listings, liquidity});
  }
  if (m_model->rowCount() > 0) m_table->selectRow(0);
  else m_inspector->clearItem();
  m_totalMetric->setText(QStringLiteral("远端命中\n%1").arg(QLocale().toString(m_totalCount)));
  m_visibleMetric->setText(QStringLiteral("当前页显示\n%1 / %2")
                               .arg(filtered.items.size()).arg(m_sourceItems.size()));
  m_priceMetric->setText(QStringLiteral("可见中位价\n%1")
                             .arg(filtered.medianPriceMinor > 0
                                      ? priceText(filtered.medianPriceMinor, m_currency)
                                      : QStringLiteral("—")));
  m_listingsMetric->setText(QStringLiteral("可见挂单总量\n%1")
                                .arg(QLocale().toString(filtered.totalListings)));
  updatePagination();
  updateFilterSummary(filtered.items.size());
}

void MarketBrowserPage::setStatus(const MarketUiStatus &status) {
  m_notice->setStatus(status);
  const bool busy = status.state == MarketViewState::Loading;
  if (busy) m_loading->show();
  else m_loading->hide();
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
  const QString symbol = Currency::displaySymbol(m_currency);
  const QSignalBlocker minimumBlocker(m_minPrice);
  const QSignalBlocker maximumBlocker(m_maxPrice);
  m_minPrice->setPrefix(symbol);
  m_maxPrice->setPrefix(symbol);
  // 数值区间不能跨币种沿用，否则 ¥100 会被误解释为 $100。
  m_minPrice->setValue(0.0);
  m_maxPrice->setValue(0.0);
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
  const QSignalBlocker searchBlocker(m_search);
  const QSignalBlocker gameBlocker(m_game);
  const QSignalBlocker sortBlocker(m_sort);
  const QSignalBlocker presetBlocker(m_preset);
  const QSignalBlocker typeBlocker(m_typeFilter);
  const QSignalBlocker minimumPriceBlocker(m_minPrice);
  const QSignalBlocker maximumPriceBlocker(m_maxPrice);
  const QSignalBlocker listingsBlocker(m_minListings);
  const QSignalBlocker pricedBlocker(m_pricedOnly);
  m_search->clear();
  m_game->setCurrentIndex(0);
  m_sort->setCurrentIndex(0);
  m_preset->setCurrentIndex(0);
  m_typeFilter->clear();
  m_minPrice->setValue(0.0);
  m_maxPrice->setValue(0.0);
  m_minListings->setValue(0);
  m_pricedOnly->setChecked(true);
  requestPage(0);
}

void MarketBrowserPage::applyPreset(int index) {
  if (index <= 0) return;
  const QStringList parts = m_preset->itemData(index).toString().split(QLatin1Char('|'));
  if (parts.size() != 2) return;
  const QSignalBlocker searchBlocker(m_search);
  const QSignalBlocker gameBlocker(m_game);
  const QSignalBlocker sortBlocker(m_sort);
  const QSignalBlocker presetBlocker(m_preset);
  m_search->setText(parts.at(1));
  const int gameIndex = m_game->findData(parts.at(0).toInt());
  m_game->setCurrentIndex(gameIndex >= 0 ? gameIndex : 0);
  m_sort->setCurrentIndex(0);
  m_preset->setCurrentIndex(0);
  m_debounce->stop();
  requestPage(0);
}

void MarketBrowserPage::updateFilterSummary(int visibleCount) {
  QStringList localFilters;
  if (!m_typeFilter->text().trimmed().isEmpty()) {
    localFilters.append(QStringLiteral("类型含“%1”").arg(m_typeFilter->text().trimmed()));
  }
  if (m_minPrice->value() > 0) {
    localFilters.append(QStringLiteral("价格 ≥ %1%2")
                            .arg(Currency::displaySymbol(m_currency))
                            .arg(m_minPrice->value(), 0, 'f', 2));
  }
  if (m_maxPrice->value() > 0) {
    localFilters.append(QStringLiteral("价格 ≤ %1%2")
                            .arg(Currency::displaySymbol(m_currency))
                            .arg(m_maxPrice->value(), 0, 'f', 2));
  }
  if (m_minListings->value() > 0) {
    localFilters.append(QStringLiteral("挂单量 ≥ %1").arg(m_minListings->value()));
  }
  if (m_pricedOnly->isChecked()) localFilters.append(QStringLiteral("有报价"));
  const QString remote = QStringLiteral("远端：%1 · %2 · %3")
                             .arg(m_search->text().trimmed().isEmpty()
                                      ? QStringLiteral("全部关键词")
                                      : QStringLiteral("“%1”").arg(m_search->text().trimmed()),
                                  scopeLabel(), m_sort->currentText());
  m_filterHint->setText(
      QStringLiteral("%1｜当前页：%2｜显示 %3 条")
          .arg(remote,
               localFilters.isEmpty() ? QStringLiteral("无二次筛选")
                                      : localFilters.join(QStringLiteral("、")))
          .arg(visibleCount));
}

void MarketBrowserPage::updatePagination() {
  const qint64 first = !m_sourceItems.isEmpty() ? m_offset + 1 : 0;
  const qint64 last = !m_sourceItems.isEmpty() ? m_offset + m_sourceItems.size() : 0;
  m_range->setText(QStringLiteral("远端第 %1–%2 项 / 共 %3 项 · 当前页显示 %4 条")
                       .arg(QLocale().toString(first), QLocale().toString(last),
                            QLocale().toString(m_totalCount),
                            QLocale().toString(m_filteredCount)));
  m_previous->setEnabled(m_offset > 0);
  m_next->setEnabled(m_offset + m_pageSize < m_totalCount);
}

QString MarketBrowserPage::scopeLabel() const {
  return m_game->currentData().toInt() == 0 ? QStringLiteral("全站") : m_game->currentText();
}
