#include "ui/widgets/QuickInspector.h"

#include <QLocale>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {

QString gameName(int appid) {
  switch (appid) {
    case 730:
      return QStringLiteral("Counter-Strike 2");
    case 570:
      return QStringLiteral("Dota 2");
    case 440:
      return QStringLiteral("Team Fortress 2");
    case 753:
      return QStringLiteral("Steam 社区物品");
    default:
      return QStringLiteral("App %1").arg(appid);
  }
}

QString moneyText(qint64 minor, const QString &currency) {
  const QString symbol = currency == QStringLiteral("USD") ? QStringLiteral("$")
                                                             : QStringLiteral("¥");
  return symbol + QLocale().toString(static_cast<double>(minor) / 100.0, 'f', 2);
}

}  // namespace

QuickInspector::QuickInspector(QWidget *parent) : QFrame(parent) {
  setObjectName(QStringLiteral("quickInspector"));
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(10);
  auto *heading = new QLabel(QStringLiteral("快速预览"), this);
  heading->setObjectName(QStringLiteral("sectionTitle"));
  layout->addWidget(heading);
  m_name = new QLabel(QStringLiteral("选择一个物品查看摘要"), this);
  m_name->setObjectName(QStringLiteral("pageTitle"));
  m_name->setTextFormat(Qt::PlainText);
  m_name->setWordWrap(true);
  layout->addWidget(m_name);
  m_game = new QLabel(this);
  m_game->setObjectName(QStringLiteral("mutedText"));
  layout->addWidget(m_game);
  m_price = new QLabel(QStringLiteral("最低挂单价 —"), this);
  m_price->setObjectName(QStringLiteral("dataValue"));
  layout->addWidget(m_price);
  m_listings = new QLabel(QStringLiteral("当前挂单量 —"), this);
  m_listings->setObjectName(QStringLiteral("dataLabel"));
  layout->addWidget(m_listings);
  m_depth = new QLabel(QStringLiteral("选择物品后按需加载深度行情"), this);
  m_depth->setObjectName(QStringLiteral("mutedText"));
  m_depth->setWordWrap(true);
  layout->addWidget(m_depth);
  layout->addStretch();
  m_freshness = new QLabel(QStringLiteral("数据来源：Steam Community Market"), this);
  m_freshness->setObjectName(QStringLiteral("mutedText"));
  m_freshness->setWordWrap(true);
  layout->addWidget(m_freshness);
  auto *actions = new QHBoxLayout();
  m_analysis = new QPushButton(QStringLiteral("价格分析"), this);
  m_analysis->setObjectName(QStringLiteral("primaryButton"));
  m_follow = new QPushButton(QStringLiteral("关注"), this);
  m_alert = new QPushButton(QStringLiteral("提醒"), this);
  actions->addWidget(m_analysis);
  actions->addWidget(m_follow);
  actions->addWidget(m_alert);
  layout->addLayout(actions);
  clearItem();

  connect(m_analysis, &QPushButton::clicked, this,
          [this]() { emit analysisRequested(m_appid, m_hashName); });
  connect(m_follow, &QPushButton::clicked, this,
          [this]() { emit followRequested(m_appid, m_hashName); });
  connect(m_alert, &QPushButton::clicked, this,
          [this]() { emit alertRequested(m_appid, m_hashName); });
}

void QuickInspector::setItem(const MarketCatalogItemView &item, const QString &currency) {
  m_appid = item.appid;
  m_hashName = item.marketHashName;
  m_name->setText(item.name);
  m_name->setToolTip(item.marketHashName);
  m_game->setText(item.typeText.isEmpty() ? gameName(item.appid)
                                          : QStringLiteral("%1 · %2")
                                                .arg(gameName(item.appid), item.typeText));
  m_price->setText(QStringLiteral("最低挂单价 %1").arg(moneyText(item.lowestSellMinor, currency)));
  m_listings->setText(QStringLiteral("当前挂单量 %1")
                          .arg(QLocale().toString(item.sellListings)));
  m_depth->setText(QStringLiteral("正在等待该物品的按需深度行情…"));
  m_freshness->setText(QStringLiteral("目录数据：Steam Community Market"));
  m_analysis->setEnabled(true);
  m_follow->setEnabled(true);
  m_alert->setEnabled(true);
}

void QuickInspector::clearItem() {
  m_appid = 0;
  m_hashName.clear();
  m_name->setText(QStringLiteral("选择一个物品查看摘要"));
  m_game->clear();
  m_price->setText(QStringLiteral("最低挂单价 —"));
  m_listings->setText(QStringLiteral("当前挂单量 —"));
  m_depth->setText(QStringLiteral("选择物品后按需加载深度行情"));
  m_freshness->setText(QStringLiteral("数据来源：Steam Community Market"));
  m_analysis->setEnabled(false);
  m_follow->setEnabled(false);
  m_alert->setEnabled(false);
}

void QuickInspector::setInspection(const MarketInspectionView &inspection) {
  if (inspection.loading) {
    m_depth->setText(QStringLiteral("正在加载该物品的深度行情…"));
    return;
  }
  if (!inspection.errorText.isEmpty()) {
    m_depth->setText(inspection.errorText);
    m_depth->setObjectName(QStringLiteral("statusError"));
  } else if (inspection.available) {
    const QString buy = inspection.bestBuyText.isEmpty() ? QStringLiteral("—")
                                                          : inspection.bestBuyText;
    const QString sell = inspection.bestSellText.isEmpty() ? QStringLiteral("—")
                                                            : inspection.bestSellText;
    m_depth->setText(QStringLiteral("买一 %1 · 卖一 %2").arg(buy, sell));
    m_depth->setObjectName(QStringLiteral("mutedText"));
  } else {
    m_depth->setText(QStringLiteral("当前没有可用的深度行情"));
    m_depth->setObjectName(QStringLiteral("mutedText"));
  }
  m_freshness->setText(QStringLiteral("深度行情查询时间：%1%2")
                           .arg(inspection.fetchedAtText.isEmpty() ? QStringLiteral("未知")
                                                                  : inspection.fetchedAtText,
                                inspection.stale ? QStringLiteral(" · 缓存") : QString()));
  style()->unpolish(m_depth);
  style()->polish(m_depth);
}

