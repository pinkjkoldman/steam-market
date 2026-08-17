#include "ui/pages/MarketOverviewPage.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ui/widgets/QuickInspector.h"
#include "ui/widgets/ScopeNotice.h"
#include "ui/widgets/WorkbenchTheme.h"
#include "utils/Currency.h"

namespace {

QFrame *makeKpi(const QString &label, QLabel **value, QWidget *parent) {
  auto *card = new QFrame(parent);
  card->setObjectName(QStringLiteral("surfaceCard"));
  auto *layout = new QVBoxLayout(card);
  layout->setContentsMargins(16, 14, 16, 14);
  auto *labelWidget = new QLabel(label, card);
  labelWidget->setObjectName(QStringLiteral("dataLabel"));
  *value = new QLabel(QStringLiteral("—"), card);
  (*value)->setObjectName(QStringLiteral("dataValue"));
  layout->addWidget(labelWidget);
  layout->addWidget(*value);
  return card;
}

QString formatCount(qint64 value) { return QLocale().toString(value); }

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

MarketOverviewPage::MarketOverviewPage(QWidget *parent) : QWidget(parent) {
  setProperty("workbench", true);
  setStyleSheet(WorkbenchTheme::styleSheet());
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(32, 24, 32, 24);
  root->setSpacing(16);

  auto *header = new QHBoxLayout();
  auto *heading = new QVBoxLayout();
  auto *title = new QLabel(QStringLiteral("市场概览"), this);
  title->setObjectName(QStringLiteral("pageTitle"));
  auto *subtitle = new QLabel(QStringLiteral("Steam 当前可检索全市场规模与热门挂单"), this);
  subtitle->setObjectName(QStringLiteral("pageSubtitle"));
  heading->addWidget(title);
  heading->addWidget(subtitle);
  m_search = new QLineEdit(this);
  m_search->setPlaceholderText(QStringLiteral("搜索 Steam 全市场"));
  m_search->setClearButtonEnabled(true);
  m_search->setMinimumWidth(280);
  m_game = new QComboBox(this);
  m_game->addItem(QStringLiteral("Steam 全站"), 0);
  m_game->addItem(QStringLiteral("Counter-Strike 2"), 730);
  m_game->addItem(QStringLiteral("Dota 2"), 570);
  m_game->addItem(QStringLiteral("Team Fortress 2"), 440);
  m_game->addItem(QStringLiteral("Steam 社区物品"), 753);
  m_browse = new QPushButton(QStringLiteral("浏览全部"), this);
  m_browse->setObjectName(QStringLiteral("primaryButton"));
  header->addLayout(heading, 1);
  header->addWidget(m_search);
  header->addWidget(m_game);
  header->addWidget(m_browse);
  root->addLayout(header);

  auto *kpis = new QGridLayout();
  kpis->setHorizontalSpacing(12);
  kpis->addWidget(makeKpi(QStringLiteral("Steam 可检索物品"), &m_totalValue, this), 0, 0);
  kpis->addWidget(makeKpi(QStringLiteral("CS2 物品"), &m_csValue, this), 0, 1);
  kpis->addWidget(makeKpi(QStringLiteral("Dota 2 物品"), &m_dotaValue, this), 0, 2);
  kpis->addWidget(makeKpi(QStringLiteral("Steam 社区物品"), &m_communityValue, this), 0, 3);
  root->addLayout(kpis);

  auto *content = new QGridLayout();
  content->setHorizontalSpacing(16);
  content->setVerticalSpacing(16);
  auto *popularCard = new QFrame(this);
  popularCard->setObjectName(QStringLiteral("surfaceCard"));
  auto *popularLayout = new QVBoxLayout(popularCard);
  popularLayout->setContentsMargins(16, 14, 16, 14);
  auto *popularTitle = new QLabel(QStringLiteral("Steam 热门物品"), popularCard);
  popularTitle->setObjectName(QStringLiteral("sectionTitle"));
  popularLayout->addWidget(popularTitle);
  m_table = new QTableView(popularCard);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setAlternatingRowColors(true);
  m_table->verticalHeader()->setVisible(false);
  m_table->verticalHeader()->setDefaultSectionSize(42);
  m_model = new QStandardItemModel(this);
  m_model->setHorizontalHeaderLabels({QStringLiteral("物品"), QStringLiteral("游戏"),
                                      QStringLiteral("最低挂单价"), QStringLiteral("挂单量")});
  m_table->setModel(m_model);
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  popularLayout->addWidget(m_table, 1);
  content->addWidget(popularCard, 0, 0, 1, 8);

  auto *distributionCard = new QFrame(this);
  distributionCard->setObjectName(QStringLiteral("surfaceCard"));
  auto *distributionLayout = new QVBoxLayout(distributionCard);
  distributionLayout->setContentsMargins(16, 14, 16, 14);
  auto *distributionTitle = new QLabel(QStringLiteral("全市场游戏分布"), distributionCard);
  distributionTitle->setObjectName(QStringLiteral("sectionTitle"));
  m_distribution = new QLabel(QStringLiteral("范围计数加载后显示"), distributionCard);
  m_distribution->setObjectName(QStringLiteral("mutedText"));
  m_distribution->setWordWrap(true);
  distributionLayout->addWidget(distributionTitle);
  distributionLayout->addWidget(m_distribution);
  distributionLayout->addStretch();
  content->addWidget(distributionCard, 0, 8, 1, 4);

  m_inspector = new QuickInspector(this);
  content->addWidget(m_inspector, 1, 0, 1, 8);
  auto *personalCard = new QFrame(this);
  personalCard->setObjectName(QStringLiteral("surfaceCard"));
  auto *personalLayout = new QVBoxLayout(personalCard);
  personalLayout->setContentsMargins(16, 14, 16, 14);
  auto *personalTitle = new QLabel(QStringLiteral("我的关注与提醒"), personalCard);
  personalTitle->setObjectName(QStringLiteral("sectionTitle"));
  m_personal = new QLabel(QStringLiteral("我的关注 0 · 提醒 0"), personalCard);
  m_personal->setObjectName(QStringLiteral("mutedText"));
  m_personal->setWordWrap(true);
  auto *personalActions = new QHBoxLayout();
  auto *watchButton = new QPushButton(QStringLiteral("管理关注"), personalCard);
  auto *alertButton = new QPushButton(QStringLiteral("管理提醒"), personalCard);
  personalActions->addWidget(watchButton);
  personalActions->addWidget(alertButton);
  personalLayout->addWidget(personalTitle);
  personalLayout->addWidget(m_personal, 1);
  personalLayout->addLayout(personalActions);
  content->addWidget(personalCard, 1, 8, 1, 4);
  content->setColumnStretch(0, 1);
  root->addLayout(content, 1);

  m_notice = new ScopeNotice(this);
  root->addWidget(m_notice);

  connect(m_search, &QLineEdit::returnPressed, this, &MarketOverviewPage::requestCurrentScope);
  connect(m_game, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int) { requestCurrentScope(); });
  connect(m_browse, &QPushButton::clicked, this, [this]() {
    emit browseAllRequested(m_search->text().trimmed(), m_game->currentData().toInt());
  });
  connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          [this]() { selectCurrentRow(); });
  connect(m_table, &QTableView::activated, this,
          [this]() { activateCurrentRow(); });
  connect(m_inspector, &QuickInspector::analysisRequested, this,
          &MarketOverviewPage::itemActivated);
  connect(m_inspector, &QuickInspector::followRequested, this,
          &MarketOverviewPage::followRequested);
  connect(m_inspector, &QuickInspector::alertRequested, this,
          &MarketOverviewPage::alertRequested);
  connect(m_notice, &ScopeNotice::retryRequested, this, &MarketOverviewPage::retryRequested);
  connect(m_notice, &ScopeNotice::openOfficialMarketRequested, this,
          &MarketOverviewPage::openOfficialMarketRequested);
  connect(watchButton, &QPushButton::clicked, this,
          &MarketOverviewPage::manageWatchlistRequested);
  connect(alertButton, &QPushButton::clicked, this,
          &MarketOverviewPage::manageAlertsRequested);
}

void MarketOverviewPage::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  if (!m_requestedOnce) {
    m_requestedOnce = true;
    emit scopeSummaryRequested();
    requestCurrentScope();
  }
}

void MarketOverviewPage::setScopeSnapshots(const QVector<MarketScopeSnapshotView> &snapshots) {
  QStringList lines;
  for (const auto &snapshot : snapshots) {
    QLabel *target = nullptr;
    QString label;
    if (snapshot.scopeKey == QStringLiteral("all")) {
      target = m_totalValue;
      label = QStringLiteral("Steam 全站");
    } else if (snapshot.scopeKey == QStringLiteral("appid:730")) {
      target = m_csValue;
      label = QStringLiteral("CS2");
    } else if (snapshot.scopeKey == QStringLiteral("appid:570")) {
      target = m_dotaValue;
      label = QStringLiteral("Dota 2");
    } else if (snapshot.scopeKey == QStringLiteral("appid:753")) {
      target = m_communityValue;
      label = QStringLiteral("Steam 社区");
    }
    if (target) target->setText(formatCount(snapshot.totalCount));
    if (!label.isEmpty()) {
      lines << QStringLiteral("%1  %2%3")
                   .arg(label, formatCount(snapshot.totalCount),
                        snapshot.origin == MarketDataOrigin::SteamCached
                            ? QStringLiteral(" · 缓存") : QString());
    }
  }
  m_distribution->setText(lines.isEmpty() ? QStringLiteral("范围计数暂不可用")
                                           : lines.join(QLatin1Char('\n')));
}

void MarketOverviewPage::setPopularPage(const MarketCatalogPageView &page) {
  m_model->removeRows(0, m_model->rowCount());
  for (const auto &item : page.items) {
    auto *name = new QStandardItem(item.name);
    name->setData(item.appid, Qt::UserRole);
    name->setData(item.marketHashName, Qt::UserRole + 1);
    name->setData(QVariant::fromValue(item), Qt::UserRole + 2);
    name->setToolTip(item.marketHashName);
    auto *price = new QStandardItem(priceText(item.lowestSellMinor, m_currency));
    price->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *listings = new QStandardItem(formatCount(item.sellListings));
    listings->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_model->appendRow({name, new QStandardItem(gameName(item.appid)), price, listings});
  }
  if (m_model->rowCount() > 0) m_table->selectRow(0);
  else m_inspector->clearItem();
  m_notice->setPageContext(page, scopeLabel(), m_currency);
}

void MarketOverviewPage::setStatus(const MarketUiStatus &status) {
  m_notice->setStatus(status);
  m_browse->setEnabled(status.state != MarketViewState::Loading);
}

void MarketOverviewPage::setInspection(const MarketInspectionView &inspection) {
  m_inspector->setInspection(inspection);
}

void MarketOverviewPage::setPersonalSummary(int watchCount, int alertCount,
                                            const QStringList &recentLines) {
  QString text = QStringLiteral("我的关注 %1 · 提醒 %2").arg(watchCount).arg(alertCount);
  if (!recentLines.isEmpty()) {
    text += QLatin1Char('\n');
    text += recentLines.mid(0, 3).join(QLatin1Char('\n'));
  }
  m_personal->setText(text);
}

void MarketOverviewPage::setCurrency(const QString &code) {
  const QString next = code.trimmed().isEmpty() ? QStringLiteral("CNY") : code.trimmed();
  if (next == m_currency) return;
  m_currency = next;
  if (m_requestedOnce) requestCurrentScope();
}

void MarketOverviewPage::requestCurrentScope() {
  emit popularPageRequested(m_search->text().trimmed(), m_game->currentData().toInt(), 0, 10,
                            QStringLiteral("popular"), m_currency);
}

void MarketOverviewPage::selectCurrentRow() {
  const QModelIndex index = m_table->currentIndex();
  if (!index.isValid()) return;
  const auto item = m_model->item(index.row(), 0)
                        ->data(Qt::UserRole + 2).value<MarketCatalogItemView>();
  m_inspector->setItem(item, m_currency);
  emit itemInspectRequested(item.appid, item.marketHashName);
}

void MarketOverviewPage::activateCurrentRow() {
  const QModelIndex index = m_table->currentIndex();
  if (!index.isValid()) return;
  emit itemActivated(m_model->item(index.row(), 0)->data(Qt::UserRole).toInt(),
                     m_model->item(index.row(), 0)->data(Qt::UserRole + 1).toString());
}

QString MarketOverviewPage::scopeLabel() const {
  return m_game->currentData().toInt() == 0 ? QStringLiteral("全站") : m_game->currentText();
}

