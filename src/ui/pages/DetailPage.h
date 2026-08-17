#pragma once

#include <QWidget>

#include "core/models/PricePoint.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QComboBox;
class MarketService;
class WatchlistService;
class AlertService;
class PriceSourceService;
class KlineService;
class OrderbookService;
class PriceChart;
class KlineChart;
class OrderbookPanel;

// 物品详情页：分时/日K/走势 + 价格卡片 + 数据统计 + 盘口 + 平台比价。
class DetailPage : public QWidget {
    Q_OBJECT

public:
    DetailPage(MarketService *market, WatchlistService *watchlist, AlertService *alerts,
               PriceSourceService *priceSources, KlineService *kline, OrderbookService *orderbook,
               QWidget *parent = nullptr);

public slots:
    void showItem(const QString &marketHashName, int appid);
    void refreshCompare();

signals:
    void backRequested();

private:
    MarketService *m_market = nullptr;
    WatchlistService *m_watchlist = nullptr;
    AlertService *m_alerts = nullptr;
    PriceSourceService *m_sources = nullptr;
    KlineService *m_kline = nullptr;
    OrderbookService *m_orderbook = nullptr;

    QString m_hashName;
    int m_appid = 730;
    QLabel *m_title = nullptr;
    QLabel *m_price = nullptr;
    QLabel *m_volume = nullptr;
    QLabel *m_stats = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_watchBtn = nullptr;
    QPushButton *m_alertBtn = nullptr;
    QComboBox *m_range = nullptr;
    PriceChart *m_trendChart = nullptr;
    PriceChart *m_minuteChart = nullptr;
    KlineChart *m_klineChart = nullptr;
    OrderbookPanel *m_orderbookPanel = nullptr;
    QTableWidget *m_compareTable = nullptr;
    QVector<PricePoint> m_history;
};
