#include "ui/pages/DetailPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
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
#include "ui/widgets/LoadingOverlay.h"
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
    m_volume->setStyleSheet(QStringLiteral("color:#B8C8DB;"));
    m_stats = new QLabel(this);
    m_stats->setWordWrap(true);
    m_stats->setStyleSheet(QStringLiteral("color:#B8C8DB;"));
    auto *cards = new QHBoxLayout();
    cards->addWidget(m_price);
    cards->addWidget(m_volume);
    cards->addWidget(m_stats);
    cards->addStretch();

    m_status = new QLabel(QStringLiteral("正在准备历史数据…"), this);
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral(
        "background:#172230;color:#B8C8DB;border:1px solid #30445D;"
        "border-radius:6px;padding:8px 10px;"));
    m_historyLoginBtn = new QPushButton(QStringLiteral("登录 Steam"), this);
    m_historyLoginBtn->setObjectName(QStringLiteral("primaryButton"));
    m_historyLoginBtn->hide();
    auto *statusRow = new QHBoxLayout();
    statusRow->addWidget(m_status, 1);
    statusRow->addWidget(m_historyLoginBtn);

    m_range = new QComboBox(this);
    m_range->addItems({QStringLiteral("1周"), QStringLiteral("1月"), QStringLiteral("3月"),
                       QStringLiteral("1年"), QStringLiteral("全部")});
    auto *chartRow = new QHBoxLayout();
    auto *rangeLabel = new QLabel(QStringLiteral("历史区间"), this);
    rangeLabel->setStyleSheet(QStringLiteral("color:#B8C8DB;"));
    chartRow->addWidget(rangeLabel);
    chartRow->addWidget(m_range);
    auto *hoverHint = new QLabel(QStringLiteral("将鼠标移到图表上查看时点价格与成交量"), this);
    hoverHint->setStyleSheet(QStringLiteral("color:#748196;"));
    chartRow->addWidget(hoverHint);
    chartRow->addStretch();
    m_historyRefreshBtn = new QPushButton(QStringLiteral("刷新官方历史"), this);
    chartRow->addWidget(m_historyRefreshBtn);

    m_trendChart = new PriceChart(this);
    m_trendChart->setMinimumHeight(240);
    m_minuteChart = new PriceChart(this);
    m_minuteChart->setMinimumHeight(240);
    m_klineChart = new KlineChart(this);
    m_klineChart->setMinimumHeight(240);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(m_trendChart, QStringLiteral("价格走势"));
    tabs->addTab(m_minuteChart, QStringLiteral("24h 历史点"));
    tabs->addTab(m_klineChart, QStringLiteral("日K（历史点聚合）"));
    tabs->setMinimumHeight(300);
    tabs->setStyleSheet(QStringLiteral(
        "QTabWidget::pane{border:1px solid #30445D;background:#101720;}"
        "QTabBar::tab{color:#A9B7C6;background:#141E2A;padding:8px 14px;"
        "border:1px solid #30445D;border-bottom:none;}"
        "QTabBar::tab:selected{color:#F2F4F8;background:#1C2A3A;}"));

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
    auto *compareLabel = new QLabel(QStringLiteral("平台比价"), this);
    compareLabel->setStyleSheet(QStringLiteral("color:#B8C8DB;"));
    compareRow->addWidget(compareLabel);
    compareRow->addStretch();
    compareRow->addWidget(refreshCompareBtn);

    auto *orderbookLabel = new QLabel(QStringLiteral("盘口（买盘=求购 / 卖盘=挂单）"), this);
    orderbookLabel->setStyleSheet(QStringLiteral("color:#B8C8DB;"));

    auto *content = new QWidget(this);
    content->setObjectName(QStringLiteral("detailContent"));
    content->setStyleSheet(QStringLiteral(
        "QWidget#detailContent{background:#0B1118;}"
        "QWidget#detailContent QLabel{color:#B8C8DB;}"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 18);
    contentLayout->setSpacing(10);
    contentLayout->addLayout(topRow);
    contentLayout->addWidget(m_title);
    contentLayout->addLayout(cards);
    contentLayout->addLayout(statusRow);
    contentLayout->addLayout(chartRow);
    contentLayout->addWidget(tabs);
    contentLayout->addWidget(orderbookLabel);
    contentLayout->addWidget(m_orderbookPanel);
    contentLayout->addLayout(compareRow);
    contentLayout->addWidget(m_compareTable);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea{background:#0B1118;border:none;}"));
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(content);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    m_loading = new LoadingOverlay(this);
    m_loading->setText(QStringLiteral("正在加载详情…"));

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
        m_klineChart->setBars(m_kline->barsFromPoints(m_history, rangeDays(index)));
    });
    connect(m_historyLoginBtn, &QPushButton::clicked, this, &DetailPage::loginRequested);
    connect(m_historyRefreshBtn, &QPushButton::clicked, this, &DetailPage::refreshHistory);
    connect(refreshCompareBtn, &QPushButton::clicked, this, &DetailPage::refreshCompare);
    connect(m_market, &MarketService::overviewUpdated, this,
            [this](const QString &hashName, const PriceOverview &overview,
                   const QString &error) {
        if (hashName != m_hashName) return;
        if (overview.priceHigh > 0 || overview.priceLow > 0) {
            const double price = overview.priceHigh > 0 ? overview.priceHigh : overview.priceLow;
            m_price->setText(QStringLiteral("最新价 %1%2")
                                 .arg(Currency::displaySymbol(CurrencyProvider::code()))
                                 .arg(price, 0, 'f', 2));
        }
        if (overview.volume >= 0) {
            m_volume->setText(QStringLiteral("24h 销量 %1").arg(overview.volume));
        }
        if (!error.isEmpty() && overview.priceLow <= 0 && overview.priceHigh <= 0) {
            m_price->setText(QStringLiteral("最新价暂不可用"));
        }
        refreshCompare();
    });
    connect(m_market, &MarketService::historyUpdated, this, &DetailPage::applyHistory);
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

void DetailPage::applyHistory(const HistorySnapshot &snapshot) {
    if (snapshot.marketHashName != m_hashName || snapshot.appid != m_appid) return;

    m_history = snapshot.points;
    m_historyRefreshBtn->setEnabled(snapshot.state != HistoryDataState::kLoading);
    m_historyLoginBtn->setVisible(snapshot.state == HistoryDataState::kAuthRequired);
    m_historyRefreshBtn->setVisible(snapshot.state != HistoryDataState::kAuthRequired);
    if (snapshot.state == HistoryDataState::kLoading) {
        m_loading->setText(QStringLiteral("正在读取 Steam 官方历史…"));
        m_loading->show();
    } else {
        m_loading->hide();
    }

    QString sourceText;
    QString statusText = snapshot.message;
    QString style = QStringLiteral(
        "background:#172230;color:#B8C8DB;border:1px solid #30445D;"
        "border-radius:6px;padding:8px 10px;");
    switch (snapshot.state) {
        case HistoryDataState::kLoading:
            sourceText = snapshot.points.isEmpty() ? QStringLiteral("Steam 官方历史")
                                                   : QStringLiteral("本地缓存（刷新中）");
            break;
        case HistoryDataState::kSteamLive:
            sourceText = QStringLiteral("Steam 官方历史 · 在线");
            statusText = QStringLiteral("Steam 在线历史 · %1 · %2 个有效点 · 获取于 %3%4")
                             .arg(snapshot.currency)
                             .arg(snapshot.points.size())
                             .arg(snapshot.fetchedAt.toLocalTime().toString(
                                 QStringLiteral("yyyy-MM-dd HH:mm:ss")))
                             .arg(snapshot.persisted ? QString() : QStringLiteral(" · 未写入缓存"));
            style = QStringLiteral(
                "background:#142B24;color:#8DDBB8;border:1px solid #275440;"
                "border-radius:6px;padding:8px 10px;");
            break;
        case HistoryDataState::kCached:
            sourceText = QStringLiteral("本地历史缓存");
            if (!snapshot.points.isEmpty()) {
                statusText = QStringLiteral("本地缓存历史 · %1 个有效点 · 最新点 %2 · %3")
                                 .arg(snapshot.points.size())
                                 .arg(snapshot.points.last().recordedAt.toLocalTime().toString(
                                     QStringLiteral("yyyy-MM-dd HH:mm")))
                                 .arg(snapshot.message);
            }
            style = QStringLiteral(
                "background:#172638;color:#A9C9EE;border:1px solid #31577D;"
                "border-radius:6px;padding:8px 10px;");
            break;
        case HistoryDataState::kAuthRequired:
            sourceText = snapshot.points.isEmpty() ? QStringLiteral("尚无官方历史")
                                                   : QStringLiteral("本地历史缓存");
            statusText = snapshot.points.isEmpty()
                             ? QStringLiteral("登录 Steam 获取官方历史；应用不会接收或保存密码")
                             : QStringLiteral("当前显示本地缓存；登录 Steam 可刷新官方历史");
            style = QStringLiteral(
                "background:#1B2940;color:#B9D2FF;border:1px solid #3C67A5;"
                "border-radius:6px;padding:8px 10px;");
            break;
        case HistoryDataState::kRateLimited:
            sourceText = QStringLiteral("Steam 官方历史");
            statusText = QStringLiteral("Steam 请求受限，请稍后手动重试");
            style = QStringLiteral(
                "background:#352815;color:#F0C77C;border:1px solid #765826;"
                "border-radius:6px;padding:8px 10px;");
            break;
        case HistoryDataState::kEmpty:
            sourceText = QStringLiteral("Steam 官方历史 · 空数据");
            statusText = QStringLiteral("Steam 本次明确返回空历史，没有生成演示数据");
            break;
        case HistoryDataState::kInvalidResponse:
            sourceText = snapshot.points.isEmpty()
                             ? QStringLiteral("Steam 官方历史 · 格式异常")
                             : QStringLiteral("本地历史缓存（官方格式异常）");
            statusText = snapshot.points.isEmpty()
                             ? QStringLiteral("Steam 历史格式发生变化，已停止写入")
                             : QStringLiteral("官方响应格式异常；已停止写入并继续显示本地缓存");
            style = QStringLiteral(
                "background:#341D22;color:#F2ADB8;border:1px solid #753A45;"
                "border-radius:6px;padding:8px 10px;");
            break;
        case HistoryDataState::kUnavailable:
            sourceText = QStringLiteral("Steam 官方历史 · 不可用");
            if (statusText.isEmpty()) statusText = QStringLiteral("网络不可用，请稍后重试");
            break;
    }
    m_status->setText(statusText);
    m_status->setStyleSheet(style);
    m_trendChart->setSourceText(sourceText);
    m_minuteChart->setSourceText(sourceText + QStringLiteral(" · 最近24小时已有点"));

    if (m_history.isEmpty()) {
        m_trendChart->setEmptyText(QStringLiteral("暂无可绘制的真实历史点"));
        m_minuteChart->setEmptyText(QStringLiteral("最近24小时暂无真实历史点"));
        m_klineChart->setBars(QVector<KlineBar>{});
    } else {
        const int days = rangeDays(m_range->currentIndex());
        m_trendChart->setPoints(m_history, days);
        m_minuteChart->setPoints(m_history, 1);
        m_klineChart->setBars(m_kline->barsFromPoints(m_history, days));
    }
    updateHistoryStats();
}

void DetailPage::updateHistoryStats() {
    if (m_history.isEmpty()) {
        m_stats->setText(QStringLiteral("历史统计：—"));
        return;
    }
    qint64 volume7d = 0;
    double amount7d = 0.0;
    double high90d = -1.0;
    double low90d = -1.0;
    bool hasVolume7d = false;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const PricePoint &point : m_history) {
        if (point.recordedAt >= now.addDays(-7) && point.hasVolume) {
            volume7d += point.volume;
            amount7d += point.price * point.volume;
            hasVolume7d = true;
        }
        if (point.recordedAt >= now.addDays(-90)) {
            high90d = high90d < 0 ? point.price : qMax(high90d, point.price);
            low90d = low90d < 0 ? point.price : qMin(low90d, point.price);
        }
    }
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    const QString volumeText = hasVolume7d ? QString::number(volume7d) : QStringLiteral("—");
    const QString amountText = hasVolume7d
                                   ? symbol + QString::number(amount7d, 'f', 0)
                                   : QStringLiteral("—");
    m_stats->setText(QStringLiteral("有效点 %1 · 7天销量 %2 · 7天成交额 %3 · 90天区间 %4%5~%6")
                         .arg(m_history.size())
                         .arg(volumeText)
                         .arg(amountText)
                         .arg(symbol)
                         .arg(low90d > 0 ? QString::number(low90d, 'f', 2)
                                        : QStringLiteral("—"))
                         .arg(high90d > 0 ? QString::number(high90d, 'f', 2)
                                         : QStringLiteral("—")));
}

void DetailPage::showItem(const QString &marketHashName, int appid) {
    m_hashName = marketHashName;
    m_appid = appid > 0 ? appid : 730;
    m_title->setText(marketHashName);
    m_price->setText(QStringLiteral("加载中…"));
    m_volume->setText(QStringLiteral("24h 销量 —"));
    m_stats->setText(QString());
    m_history.clear();
    m_status->setText(QStringLiteral("正在读取本地缓存与 Steam 官方历史…"));
    m_historyLoginBtn->hide();
    m_historyRefreshBtn->setEnabled(false);
    m_trendChart->setEmptyText(QStringLiteral("正在加载历史数据…"));
    m_minuteChart->setEmptyText(QStringLiteral("正在加载最近24小时历史点…"));
    m_klineChart->setBars(QVector<KlineBar>{});
    m_watchBtn->setText(m_watchlist->isWatched(marketHashName) ? QStringLiteral("✓ 已加入自选")
                                                               : QStringLiteral("★ 加入自选"));
    m_market->fetchDetail(marketHashName, m_appid);
    m_orderbook->loadOrderbook(marketHashName, m_appid);
    m_loading->show();
    refreshCompare();
}

void DetailPage::refreshHistory() {
    if (m_hashName.isEmpty()) return;
    m_loading->setText(QStringLiteral("正在刷新 Steam 官方历史…"));
    m_loading->show();
    m_market->refreshHistory(m_hashName, m_appid);
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
