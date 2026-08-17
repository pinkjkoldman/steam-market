#include "ui/widgets/PriceChart.h"

#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QDateTimeAxis>
#include <QLineSeries>
#include <QPainter>
#include <QValueAxis>
#include <QVBoxLayout>

PriceChart::PriceChart(QWidget *parent) : QWidget(parent) {
    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->setBackgroundRoundness(6);
    m_view = new QChartView(m_chart, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void PriceChart::setCurrencySymbol(const QString &symbol) {
    m_symbol = symbol;
    rebuild();
}

void PriceChart::setEmptyText(const QString &text) {
    m_chart->setTitle(text);
    m_chart->removeAllSeries();
}

void PriceChart::setPoints(const QVector<PricePoint> &points, int maxDays) {
    m_points = points;
    m_maxDays = maxDays;
    rebuild();
}

void PriceChart::rebuild() {
    m_chart->removeAllSeries();
    m_chart->setTitle(QString());
    if (m_points.size() < 2) {
        m_chart->setTitle(QStringLiteral("历史数据不足，无法绘制走势图"));
        return;
    }

    QDateTime cutoff;
    if (m_maxDays > 0) {
        cutoff = QDateTime::currentDateTimeUtc().addDays(-m_maxDays);
    }
    QVector<PricePoint> filtered;
    for (const PricePoint &p : m_points) {
        if (cutoff.isValid() && p.recordedAt < cutoff) continue;
        filtered.append(p);
    }
    if (filtered.size() < 2) {
        m_chart->setTitle(QStringLiteral("所选区间数据不足"));
        return;
    }

    auto *line = new QLineSeries();
    auto *bars = new QBarSeries();
    auto *barSet = new QBarSet(QStringLiteral("销量"));
    line->setName(QStringLiteral("价格"));
    line->setColor(QColor(QStringLiteral("#3D7EFF")));
    barSet->setColor(QColor(0x46, 0xA7, 0x58, 110));
    QStringList categories;
    for (const PricePoint &p : filtered) {
        line->append(p.recordedAt.toMSecsSinceEpoch(), p.price);
        barSet->append(p.volume);
        categories << p.recordedAt.toString(QStringLiteral("MM-dd"));
    }
    bars->append(barSet);

    m_chart->addSeries(bars);
    m_chart->addSeries(line);
    m_chart->createDefaultAxes();

    auto *axisX = new QDateTimeAxis();
    axisX->setFormat(QStringLiteral("MM-dd"));
    axisX->setTickCount(qMin(6, filtered.size()));
    axisX->setTitleText(QStringLiteral("时间"));
    m_chart->addAxis(axisX, Qt::AlignBottom);
    line->attachAxis(axisX);
    bars->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    axisY->setTitleText(QStringLiteral("%1价格").arg(m_symbol));
    axisY->setLabelFormat(QStringLiteral("%.2f"));
    m_chart->addAxis(axisY, Qt::AlignLeft);
    line->attachAxis(axisY);

    auto *axisY2 = new QValueAxis();
    axisY2->setTitleText(QStringLiteral("销量"));
    m_chart->addAxis(axisY2, Qt::AlignRight);
    bars->attachAxis(axisY2);

    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_view->update();
}
