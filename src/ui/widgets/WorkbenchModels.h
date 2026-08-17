#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>

enum class MarketDataOrigin { SteamLive, SteamCached };

enum class MarketViewState {
  Initial,
  Loading,
  Ready,
  Empty,
  OfflineNoCache,
  RateLimited,
  SchemaChanged,
  Error
};

struct MarketCatalogItemView {
  int appid = 0;
  QString marketHashName;
  QString name;
  QString typeText;
  QString iconUrl;
  qint64 lowestSellMinor = 0;
  qint64 sellListings = 0;
};

struct MarketCatalogPageView {
  int offset = 0;
  int pageSize = 10;
  qint64 totalCount = 0;
  QVector<MarketCatalogItemView> items;
  QDateTime fetchedAt;
  MarketDataOrigin origin = MarketDataOrigin::SteamLive;
  bool stale = false;
  QString sourceLabel = QStringLiteral("steam_community_market_public_page");
};

struct MarketScopeSnapshotView {
  QString scopeKey;
  qint64 totalCount = 0;
  QDateTime fetchedAt;
  MarketDataOrigin origin = MarketDataOrigin::SteamLive;
};

struct MarketUiStatus {
  MarketViewState state = MarketViewState::Initial;
  QString message;
  qint64 retryAfterMs = 0;
};

struct MarketInspectionView {
  bool available = false;
  bool loading = false;
  bool stale = false;
  QString lowestSellText;
  QString sellListingsText;
  QString bestBuyText;
  QString bestSellText;
  QString fetchedAtText;
  QString errorText;
};

Q_DECLARE_METATYPE(MarketDataOrigin)
Q_DECLARE_METATYPE(MarketViewState)
Q_DECLARE_METATYPE(MarketCatalogItemView)
Q_DECLARE_METATYPE(MarketCatalogPageView)
Q_DECLARE_METATYPE(MarketScopeSnapshotView)
Q_DECLARE_METATYPE(MarketUiStatus)
Q_DECLARE_METATYPE(MarketInspectionView)

