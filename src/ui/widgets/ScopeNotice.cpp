#include "ui/widgets/ScopeNotice.h"

#include <QLocale>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>

ScopeNotice::ScopeNotice(QWidget *parent) : QFrame(parent) {
  setObjectName(QStringLiteral("scopeNotice"));
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(12);
  auto *text = new QVBoxLayout();
  text->setSpacing(2);
  m_scope = new QLabel(QStringLiteral("Steam Community Market · 全站 · CNY"), this);
  m_scope->setTextFormat(Qt::PlainText);
  m_freshness = new QLabel(QStringLiteral("等待查询"), this);
  m_freshness->setObjectName(QStringLiteral("mutedText"));
  m_freshness->setTextFormat(Qt::PlainText);
  text->addWidget(m_scope);
  text->addWidget(m_freshness);
  layout->addLayout(text, 1);

  m_status = new QLabel(this);
  m_status->setTextFormat(Qt::PlainText);
  m_status->setWordWrap(true);
  layout->addWidget(m_status);
  m_retry = new QPushButton(QStringLiteral("重试"), this);
  m_retry->setVisible(false);
  m_official = new QPushButton(QStringLiteral("打开 Steam 官方市场"), this);
  m_official->setObjectName(QStringLiteral("linkButton"));
  m_official->setVisible(false);
  layout->addWidget(m_retry);
  layout->addWidget(m_official);
  connect(m_retry, &QPushButton::clicked, this, &ScopeNotice::retryRequested);
  connect(m_official, &QPushButton::clicked, this, &ScopeNotice::openOfficialMarketRequested);
}

void ScopeNotice::setPageContext(const MarketCatalogPageView &page,
                                 const QString &scopeLabel, const QString &currency) {
  m_scope->setText(QStringLiteral("Steam Community Market · %1 · %2")
                       .arg(scopeLabel, currency));
  const QString time = page.fetchedAt.isValid()
                           ? QLocale().toString(page.fetchedAt.toLocalTime(),
                                               QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                           : QStringLiteral("未知时间");
  if (page.origin == MarketDataOrigin::SteamCached) {
    m_freshness->setText(QStringLiteral("正在显示缓存数据 · 查询时间 %1%2")
                             .arg(time, page.stale ? QStringLiteral(" · 已过期") : QString()));
    m_freshness->setObjectName(QStringLiteral("statusWarning"));
  } else {
    m_freshness->setText(QStringLiteral("Steam 实时响应 · 查询时间 %1").arg(time));
    m_freshness->setObjectName(QStringLiteral("statusLive"));
  }
  style()->unpolish(m_freshness);
  style()->polish(m_freshness);
}

void ScopeNotice::setStatus(const MarketUiStatus &status) {
  QString message = status.message;
  if (message.isEmpty()) {
    switch (status.state) {
      case MarketViewState::Loading:
        message = QStringLiteral("正在向 Steam 请求当前页…");
        break;
      case MarketViewState::RateLimited:
        message = status.retryAfterMs > 0
                      ? QStringLiteral("Steam 请求受限 · %1 秒后可重试")
                            .arg((status.retryAfterMs + 999) / 1000)
                      : QStringLiteral("Steam 请求受限 · 请稍后重试");
        break;
      case MarketViewState::OfflineNoCache:
        message = QStringLiteral("网络不可用，且当前查询没有缓存");
        break;
      case MarketViewState::SchemaChanged:
        message = QStringLiteral("Steam 响应结构发生变化，暂时无法解析");
        break;
      default:
        break;
    }
  }
  m_status->setText(message);
  const bool warning = status.state == MarketViewState::RateLimited;
  const bool error = status.state == MarketViewState::OfflineNoCache
                     || status.state == MarketViewState::SchemaChanged
                     || status.state == MarketViewState::Error;
  m_status->setObjectName(error ? QStringLiteral("statusError")
                                : warning ? QStringLiteral("statusWarning")
                                          : QStringLiteral("mutedText"));
  m_retry->setVisible(error || warning);
  m_official->setVisible(status.state == MarketViewState::OfflineNoCache
                         || status.state == MarketViewState::SchemaChanged);
  style()->unpolish(m_status);
  style()->polish(m_status);
}

