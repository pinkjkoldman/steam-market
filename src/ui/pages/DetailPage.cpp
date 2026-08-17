#include "ui/pages/DetailPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/models/PricePoint.h"
#include "core/services/AlertService.h"
#include "core/services/KlineService.h"
#include "core/services/MarketService.h"
#include "core/services/OrderbookService.h"
#include "core/services/PriceSourceService.h"
#include "core/services/WatchlistService.h"
#include "ui/widgets/KlineChart.h"
#include "ui/widgets/OrderbookPanel.h"
#include "ui/widgets/PriceChart.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"

namespace {
int rangeDays(int index) {
    switch (index) {
        case 0: return 7;
        case 1: return 30;
        case 2: return 90;
        case 3: return 365;
        default: return 0;  // 全部
    }
}
}  // namespace

DetailPage::DetailPage(MarketService *market, WatchlistService *watchlist, AlertService *alerts,
                       PriceSourceService *priceSources, KlineService *kline,
                       OrderbookService *orderbook, QWidget *parent)
    : QWidget(parent), m_market(market), m_watchlist(watchlist), m_alerts(alerts),
      m_sources(priceSources), m_kline(kline), m_orderbook(orderbook) {
    auto *backBtn = new QPushButton(QStringLiteral("← 返回"), this);
    m_watchBtn = new QPushButton(QStringLiteral("★ 加入自选"), this);
    m_alertBtn = new QPushButton(QStringLiteral("⏰ 设置提醒"), this);
    auto *topRow = new QHBoxLayout();
    topRow->addWidget(backBtn);
    topRow->addStretch();
    topRow->addWidget(m_watchBtn);
    topRow->addWidget(m_alertBtn);

    m_title = new QLabel(QStringLiteral("—"), this);
    m_title->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: bold; color: #F2F4F8;"));
    m_price = new QLabel(QStringLiteral("—"), this);
    m_price->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: bold; color: #5A93FF;"));
    m_volume = new QLabel(this);
    m_stats = new QLabel(this);
    m_stats->setWordWrap(true);
    auto *cards = new QHBoxLayout();
    cards->addWidget(m_price);
    cards->addWidget(m_volume);
    cards->addWidget(m_stats);
    cards->addStretch();

    m_range = new QComboBox(this);
    m_range->addItems({QStringLiteral("1周"), QStringLiteral("1月"), QStringLiteral("3月"),
                       QStringLiteral("1年"), QStringLiteral("全部")});
    auto *chartRow = new QHBoxLayout();
    chartRow->addStretch();
    chartRow->addWidget(m_range);
    chartRow->addStretch();

    m_trendChart = new PriceChart(this);
    m_trendChart->setMinimumHeight(240);
    m_minuteChart = new PriceChart(this);
    m_minuteChart->setMinimumHeight(240);
    m_klineChart = new KlineChart(this);
    m_klineChart->setMinimumHeight(240);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(m_trendChart, QStringLiteral("走势"));
    tabs->addTab(m_minuteChart, QStringLiteral("分时"));
    tabs->addTab(m_klineChart, QStringLiteral("日K"));
    tabs->setMinimumHeight(300);

    m_orderbookPanel = new OrderbookPanel(this);
    m_orderbookPanel->setMaximumHeight(280);

    m_compareTable = new QTableWidget(0, 3, this);
    m_compareTable->setHorizontalHeaderLabels(
        {QStringLiteral("平台"), QStringLiteral("价格"), QStringLiteral("更新时间")});
    m_compareTable->horizontalHeader()->setStretchLastSection(true);
    m_compareTable->verticalHeader()->setVisible(false);
    m_compareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_compareTable->setMaximumHeight(140);
    auto *refreshCompareBtn = new QPushButton(QStringLiteral("刷新比价"), this);
    auto *compareRow = new QHBoxLayout();
    compareRow->addWidget(new QLabel(QStringLiteral("平台比价"), this));
    compareRow->addStretch();
    compareRow->addWidget(refreshCompareBtn);

    m_status = new QLabel(this);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(m_title);
    layout->addLayout(cards);
    layout->addLayout(chartRow);
    layout->addWidget(tabs, 1);
    layout->addWidget(new QLabel(QStringLiteral("盘口（买盘=求购 / 卖盘=挂单）"), this));
    layout->addWidget(m_orderbookPanel);
    layout->addLayout(compareRow);
    layout->addWidget(m_compareTable);
    layout->addWidget(m_status);

    connect(backBtn, &QPushButton::clicked, this, &DetailPage::backRequested);
    connect(m_watchBtn, &QPushButton::clicked, this, [this]() {
        if (!m_watchlist->isWatched(m_hashName)) {
            m_watchlist->add(m_hashName, m_appid, QString());
            m_watchBtn->setText(QStringLiteral("✓ 已加入自选"));
        } else {
            m_watchlist->remove(m_hashName);
            m_watchBtn->setText(QStringLiteral("★ 加入自选"));
        }
    });
    connect(m_alertBtn, &QPushButton::clicked, this, [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("设置价格提醒"));
        auto *form = new QFormLayout(&dlg);
        auto *typeBox = new QComboBox(&dlg);
        typeBox->addItems({QStringLiteral("低于阈值"), QStringLiteral("高于阈值"),
                           QStringLiteral("24h 涨跌幅超过")});
        auto *valueEdit = new QLineEdit(&dlg);
        auto *enabledBox = new QCheckBox(QStringLiteral("启用"), &dlg);
        enabledBox->setChecked(true);
        form->addRow(QStringLiteral("条件"), typeBox);
        form->addRow(QStringLiteral("阈值"), valueEdit);
        form->addRow(enabledBox);
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
        alert.marketHashName = m_hashName;
        alert.appid = m_appid;
        alert.enabled = enabledBox->isChecked();
        if (typeBox->currentIndex() == 0) {
            alert.conditionType = Alert::Condition::kBelow;
            alert.thresholdValue = value;
        } else if (typeBox->currentIndex() == 1) {
            alert.conditionType = Alert::Condition::kAbove;
            alert.thresholdValue = value;
        } else {
            alert.conditionType = Alert::Condition::kPercent24h;
            alert.percentValue = value;
        }
        m_alerts->add(alert);
        QMessageBox::information(this, QStringLiteral("提醒已创建"),
                                 QStringLiteral("已为该物品创建价格提醒"));
    });
    connect(m_range, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_trendChart->setPoints(m_history, rangeDays(index));
        m_klineChart->setBars(m_kline->dailyBars(m_hashName, m_appid, CurrencyProvider::code(),
                                                  rangeDays(index)));
    });
    connect(refreshCompareBtn, &QPushButton::clicked, this, &DetailPage::refreshCompare);
    connect(m_market, &MarketService::detailReady, this,
            [this](const QString &hashName, const PriceOverview &overview,
                   const QVector<PricePoint> &history, const QString &error) {
        if (hashName != m_hashName) return;
        m_history = history;
        const int days = rangeDays(m_range->currentIndex());
        m_trendChart->setPoints(history, days);
        m_minuteChart->setPoints(
            m_kline->minutePoints(hashName, m_appid, CurrencyProvider::code()), 0);
        m_klineChart->setBars(
            m_kline->dailyBars(hashName, m_appid, CurrencyProvider::code(), days));
        if (overview.priceHigh > 0 || overview.priceLow > 0) {
            const double price = overview.priceHigh > 0 ? overview.priceHigh : overview.priceLow;
            m_price->setText(QStringLiteral("最新价 %1%2")
                                 .arg(Currency::displaySymbol(CurrencyProvider::code()))
                                 .arg(price, 0, 'f', 2));
        }
        if (overview.volume >= 0) {
            m_volume->setText(QStringLiteral("24h 销量 %1").arg(overview.volume));
        }
        // 数据统计：7 天销量/成交额/90 天区间
        qint64 volume7d = 0;
        double amount7d = 0.0, high90d = -1.0, low90d = -1.0;
        const QDateTime now = QDateTime::currentDateTimeUtc();
        for (const PricePoint &p : history) {
            if (p.recordedAt >= now.addDays(-7)) {
                volume7d += p.volume;
                amount7d += p.price * p.volume;
            }
            if (p.recordedAt >= now.addDays(-90)) {
                high90d = high90d < 0 ? p.price : qMax(high90d, p.price);
                low90d = low90d < 0 ? p.price : qMin(low90d, p.price);
            }
        }
        const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
        m_stats->setText(QStringLiteral("7天销量 %1 · 7天成交额 %2%3 · 90天区间 %2%4~%5")
                             .arg(volume7d)
                             .arg(symbol)
                             .arg(amount7d, 0, 'f', 0)
                             .arg(low90d > 0 ? QStringLiteral("%1").arg(low90d, 0, 'f', 2)
                                             : QStringLiteral("—"))
                             .arg(high90d > 0 ? QStringLiteral("%1").arg(high90d, 0, 'f', 2)
                                              : QStringLiteral("—")));
        m_status->setText(error.isEmpty()
                              ? (overview.stale ? QStringLiteral("离线模式：展示缓存数据")
                                                : QStringLiteral("数据已更新"))
                              : QStringLiteral("数据刷新失败：%1").arg(error));
        refreshCompare();
    });
    connect(m_orderbook, &OrderbookService::orderbookReady, this,
            [this](const QString &hashName, const Orderbook &book, const QString &error) {
        if (hashName == m_hashName) {
            m_orderbookPanel->setOrderbook(book, error);
        }
    });
    connect(m_sources, &PriceSourceService::compareUpdated, this, [this](const QString &) {
        refreshCompare();
    });
}

void DetailPage::showItem(const QString &marketHashName, int appid) {
    m_hashName = marketHashName;
    m_appid = appid > 0 ? appid : 730;
    m_title->setText(marketHashName);
    m_price->setText(QStringLiteral("加载中…"));
    m_stats->setText(QString());
    m_watchBtn->setText(m_watchlist->isWatched(marketHashName) ? QStringLiteral("✓ 已加入自选")
                                                               : QStringLiteral("★ 加入自选"));
    m_market->fetchDetail(marketHashName, m_appid);
    m_orderbook->loadOrderbook(marketHashName, m_appid);
    refreshCompare();
}

void DetailPage::refreshCompare() {
    if (m_hashName.isEmpty()) return;
    const QVector<PlatformPrice> prices = m_sources->compare(m_hashName);
    m_compareTable->setRowCount(prices.size() + 1);
    int row = 0;
    const PriceOverview snap = m_market->cachedOverview(m_hashName, m_appid);
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    m_compareTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("Steam")));
    m_compareTable->setItem(row, 1,
                            new QTableWidgetItem(snap.priceHigh > 0
                                                     ? symbol + QString::number(snap.priceHigh, 'f', 2)
                                                     : QStringLiteral("—")));
    m_compareTable->setItem(row, 2,
                            new QTableWidgetItem(snap.updatedAt.isValid()
                                                     ? snap.updatedAt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                                     : QStringLiteral("—")));
    ++row;
    for (const PlatformPrice &p : prices) {
        m_compareTable->setItem(row, 0, new QTableWidgetItem(p.platform));
        m_compareTable->setItem(row, 1,
                                new QTableWidgetItem(p.hasPrice
                                                         ? symbol + QString::number(p.price, 'f', 2)
                                                         : QStringLiteral("无数据")));
        m_compareTable->setItem(row, 2,
                                new QTableWidgetItem(p.updatedAt.isValid()
                                                         ? p.updatedAt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                                         : QStringLiteral("—")));
        ++row;
    }
}
