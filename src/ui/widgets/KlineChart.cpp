#include "ui/widgets/KlineChart.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QPainter>
#include <QValueAxis>
#include <QVBoxLayout>

#include "utils/ThemeProvider.h"

KlineChart::KlineChart(QWidget *parent) : QWidget(parent) {
    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->setBackgroundRoundness(6);
    m_view = new QChartView(m_chart, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void KlineChart::setBars(const QVector<KlineBar> &bars) {
    m_bars = bars;
    rebuild();
}

void KlineChart::rebuild() {
    m_chart->removeAllSeries();
    m_chart->setTitle(QString());
    if (m_bars.size() < 2) {
        m_chart->setTitle(QStringLiteral("K线数据不足，无法绘制"));
        return;
    }

    QStringList categories;
    double minPrice = m_bars.first().low;
    double maxPrice = m_bars.first().high;
    auto *candles = new QCandlestickSeries();
    candles->setIncreasingColor(QColor(ThemeProvider::upColor()));
    candles->setDecreasingColor(QColor(ThemeProvider::downColor()));
    candles->setName(QStringLiteral("价格"));

    auto *volBarSet = new QBarSet(QStringLiteral("成交量"));
    volBarSet->setColor(QColor(0x3D, 0x7E, 0xFF, 90));
    auto *volumeSeries = new QBarSeries();

    auto *ma5Series = new QLineSeries();
    auto *ma10Series = new QLineSeries();
    auto *ma20Series = new QLineSeries();
    ma5Series->setColor(QColor(QStringLiteral("#F5A623")));
    ma10Series->setColor(QColor(QStringLiteral("#3D7EFF")));
    ma20Series->setColor(QColor(QStringLiteral("#9AA3B2")));

    for (int i = 0; i < m_bars.size(); ++i) {
        const KlineBar &bar = m_bars.at(i);
        categories << bar.date.toString(QStringLiteral("MM-dd"));
        auto *set = new QCandlestickSet(bar.open, bar.high, bar.low, bar.close);
        candles->append(set);
        volBarSet->append(bar.volume);
        minPrice = qMin(minPrice, bar.low);
        maxPrice = qMax(maxPrice, bar.high);
        if (bar.ma5 > 0) ma5Series->append(i, bar.ma5);
        if (bar.ma10 > 0) ma10Series->append(i, bar.ma10);
        if (bar.ma20 > 0) ma20Series->append(i, bar.ma20);
    }
    volumeSeries->append(volBarSet);

    m_chart->addSeries(candles);
    m_chart->addSeries(volumeSeries);
    m_chart->addSeries(ma5Series);
    m_chart->addSeries(ma10Series);
    m_chart->addSeries(ma20Series);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    candles->attachAxis(axisX);
    volumeSeries->attachAxis(axisX);
    ma5Series->attachAxis(axisX);
    ma10Series->attachAxis(axisX);
    ma20Series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    const double pad = (maxPrice - minPrice) * 0.05 + 0.01;
    axisY->setRange(minPrice - pad, maxPrice + pad);
    axisY->setLabelFormat(QStringLiteral("%.2f"));
    m_chart->addAxis(axisY, Qt::AlignLeft);
    candles->attachAxis(axisY);
    ma5Series->attachAxis(axisY);
    ma10Series->attachAxis(axisY);
    ma20Series->attachAxis(axisY);

    auto *axisY2 = new QValueAxis();
    axisY2->setTitleText(QStringLiteral("成交量"));
    m_chart->addAxis(axisY2, Qt::AlignRight);
    volumeSeries->attachAxis(axisY2);

    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_view->update();
}
