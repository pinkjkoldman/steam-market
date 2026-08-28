#include "ui/widgets/PriceChart.h"

#include <QAbstractAxis>
#include <QAreaSeries>
#include <QChart>
#include <QChartView>
#include <QDateTimeAxis>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLineSeries>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QScatterSeries>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QXYSeries>

#include <algorithm>
#include <cmath>
#include <limits>

PriceChart::PriceChart(QWidget *parent) : QWidget(parent) {
    m_chart = new QChart();
    m_chart->setTheme(QChart::ChartThemeDark);
    m_chart->legend()->hide();
    m_chart->setBackgroundRoundness(6);
    m_chart->setBackgroundBrush(QColor(QStringLiteral("#121A24")));
    m_chart->setPlotAreaBackgroundBrush(QColor(QStringLiteral("#101720")));
    m_chart->setPlotAreaBackgroundVisible(true);
    m_view = new QChartView(m_chart, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setMouseTracking(true);
    m_view->viewport()->setMouseTracking(true);
    m_view->viewport()->installEventFilter(this);

    m_verticalCrosshair = new QFrame(m_view->viewport());
    m_horizontalCrosshair = new QFrame(m_view->viewport());
    m_hoverPoint = new QFrame(m_view->viewport());
    m_hoverLabel = new QLabel(m_view->viewport());
    for (QWidget *overlay : {static_cast<QWidget *>(m_verticalCrosshair),
                             static_cast<QWidget *>(m_horizontalCrosshair),
                             static_cast<QWidget *>(m_hoverPoint),
                             static_cast<QWidget *>(m_hoverLabel)}) {
        overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
        overlay->hide();
    }
    m_verticalCrosshair->setStyleSheet(QStringLiteral("background:rgba(232,234,237,140);"));
    m_horizontalCrosshair->setStyleSheet(QStringLiteral("background:rgba(232,234,237,140);"));
    m_hoverPoint->setStyleSheet(QStringLiteral(
        "background:#B7D0FF;border:2px solid #5A93FF;border-radius:5px;"));
    m_hoverLabel->setMaximumWidth(260);
    m_hoverLabel->setStyleSheet(QStringLiteral(
        "QLabel{background:#0B1118;color:#F2F4F8;border:1px solid #5A93FF;"
        "border-radius:6px;padding:7px 9px;}"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void PriceChart::setCurrencySymbol(const QString &symbol) {
    m_symbol = symbol;
    rebuild();
}

void PriceChart::setSourceText(const QString &text) {
    m_sourceText = text;
    rebuild();
}

void PriceChart::setEmptyText(const QString &text) {
    m_points.clear();
    clearChart();
    m_chart->setTitle(text);
}

void PriceChart::setPoints(const QVector<PricePoint> &points, int maxDays) {
    m_points = points;
    m_maxDays = maxDays;
    rebuild();
}

void PriceChart::clearChart() {
    hideHover();
    m_priceSeries = nullptr;
    m_displayedPoints.clear();
    m_chart->removeAllSeries();
    const QList<QAbstractAxis *> axes = m_chart->axes();
    for (QAbstractAxis *axis : axes) {
        m_chart->removeAxis(axis);
        delete axis;
    }
}

void PriceChart::rebuild() {
    clearChart();
    m_chart->setTitle(QString());

    QDateTime cutoff;
    if (m_maxDays > 0) {
        cutoff = QDateTime::currentDateTimeUtc().addDays(-m_maxDays);
    }
    QVector<PricePoint> filtered;
    for (const PricePoint &p : m_points) {
        if (!p.recordedAt.isValid() || !std::isfinite(p.price) || p.price <= 0.0) continue;
        if (cutoff.isValid() && p.recordedAt < cutoff) continue;
        filtered.append(p);
    }
    std::sort(filtered.begin(), filtered.end(), [](const PricePoint &left, const PricePoint &right) {
        return left.recordedAt < right.recordedAt;
    });
    if (filtered.isEmpty()) {
        m_chart->setTitle(m_sourceText.isEmpty()
                              ? QStringLiteral("所选区间暂无真实历史点")
                              : m_sourceText + QStringLiteral(" · 所选区间暂无历史点"));
        return;
    }
    m_displayedPoints = filtered;

    QXYSeries *priceSeries = nullptr;
    if (filtered.size() == 1) {
        auto *point = new QScatterSeries();
        point->setMarkerSize(9.0);
        point->setColor(QColor(QStringLiteral("#B7D0FF")));
        point->setBorderColor(QColor(QStringLiteral("#5A93FF")));
        priceSeries = point;
    } else {
        auto *line = new QLineSeries();
        line->setColor(QColor(QStringLiteral("#5A93FF")));
        line->setPen(QPen(QColor(QStringLiteral("#5A93FF")), 2.0));
        priceSeries = line;
    }
    priceSeries->setName(QStringLiteral("价格"));
    m_priceSeries = priceSeries;

    auto *volumeUpper = new QLineSeries();
    auto *volumeLower = new QLineSeries();
    bool hasVolume = false;
    double maxVolume = 0.0;
    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();
    for (const PricePoint &p : filtered) {
        const qreal x = p.recordedAt.toMSecsSinceEpoch();
        priceSeries->append(x, p.price);
        minPrice = qMin(minPrice, p.price);
        maxPrice = qMax(maxPrice, p.price);
        if (p.hasVolume || p.volume > 0) {
            volumeUpper->append(x, p.volume);
            volumeLower->append(x, 0.0);
            maxVolume = qMax(maxVolume, static_cast<double>(p.volume));
            hasVolume = true;
        }
    }
    m_chart->addSeries(priceSeries);

    QAreaSeries *volumeArea = nullptr;
    if (hasVolume) {
        volumeArea = new QAreaSeries(volumeUpper, volumeLower);
        volumeArea->setName(QStringLiteral("成交量"));
        volumeArea->setColor(QColor(90, 147, 255, 72));
        volumeArea->setBorderColor(QColor(90, 147, 255, 130));
        m_chart->addSeries(volumeArea);
    } else {
        delete volumeUpper;
        delete volumeLower;
    }

    auto *axisX = new QDateTimeAxis();
    const qint64 firstMs = filtered.first().recordedAt.toMSecsSinceEpoch();
    const qint64 lastMs = filtered.last().recordedAt.toMSecsSinceEpoch();
    const qint64 padMs = firstMs == lastMs ? 60 * 60 * 1000 : 0;
    axisX->setRange(QDateTime::fromMSecsSinceEpoch(firstMs - padMs),
                    QDateTime::fromMSecsSinceEpoch(lastMs + padMs));
    axisX->setFormat(m_maxDays > 0 && m_maxDays <= 1 ? QStringLiteral("HH:mm")
                                                     : QStringLiteral("MM-dd"));
    axisX->setTickCount(qBound(2, filtered.size(), 6));
    axisX->setTitleText(QStringLiteral("时间"));
    m_chart->addAxis(axisX, Qt::AlignBottom);
    priceSeries->attachAxis(axisX);
    if (volumeArea) volumeArea->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    axisY->setTitleText(QStringLiteral("%1价格").arg(m_symbol));
    axisY->setLabelFormat(QStringLiteral("%.2f"));
    const double pricePad = qMax(0.01, (maxPrice - minPrice) * 0.08);
    axisY->setRange(qMax(0.0, minPrice - pricePad), maxPrice + pricePad);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    priceSeries->attachAxis(axisY);

    if (volumeArea) {
        auto *axisY2 = new QValueAxis();
        axisY2->setTitleText(QStringLiteral("成交量"));
        axisY2->setLabelFormat(QStringLiteral("%.0f"));
        axisY2->setRange(0.0, qMax(1.0, maxVolume * 1.15));
        m_chart->addAxis(axisY2, Qt::AlignRight);
        volumeArea->attachAxis(axisY2);
    }

    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    QString title = m_sourceText;
    if (filtered.size() == 1) {
        if (!title.isEmpty()) title += QStringLiteral(" · ");
        title += QStringLiteral("仅 1 个有效点，暂不能形成趋势");
    }
    m_chart->setTitle(title);
    m_view->update();
}

bool PriceChart::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_view->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            updateHover(static_cast<QMouseEvent *>(event)->position().toPoint());
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::Resize) {
            hideHover();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PriceChart::updateHover(const QPoint &position) {
    if (!m_priceSeries || m_displayedPoints.isEmpty()) {
        hideHover();
        return;
    }

    const QRectF plotArea = m_chart->plotArea();
    if (!plotArea.contains(position)) {
        hideHover();
        return;
    }

    const qint64 targetMs = qRound64(
        m_chart->mapToValue(QPointF(position), m_priceSeries).x());
    auto after = std::lower_bound(
        m_displayedPoints.cbegin(), m_displayedPoints.cend(), targetMs,
        [](const PricePoint &point, qint64 timestamp) {
            return point.recordedAt.toMSecsSinceEpoch() < timestamp;
        });
    int pointIndex = 0;
    if (after == m_displayedPoints.cend()) {
        pointIndex = m_displayedPoints.size() - 1;
    } else if (after == m_displayedPoints.cbegin()) {
        pointIndex = 0;
    } else {
        const int nextIndex = static_cast<int>(after - m_displayedPoints.cbegin());
        const qint64 nextDistance = qAbs(after->recordedAt.toMSecsSinceEpoch() - targetMs);
        const qint64 previousDistance = qAbs(
            m_displayedPoints.at(nextIndex - 1).recordedAt.toMSecsSinceEpoch() - targetMs);
        pointIndex = previousDistance <= nextDistance ? nextIndex - 1 : nextIndex;
    }

    const PricePoint &point = m_displayedPoints.at(pointIndex);
    const QPointF marker = m_chart->mapToPosition(
        QPointF(point.recordedAt.toMSecsSinceEpoch(), point.price), m_priceSeries);
    if (!plotArea.adjusted(-1, -1, 1, 1).contains(marker)) {
        hideHover();
        return;
    }

    const int markerX = qRound(marker.x());
    const int markerY = qRound(marker.y());
    m_verticalCrosshair->setGeometry(markerX, qRound(plotArea.top()), 1,
                                     qRound(plotArea.height()));
    m_horizontalCrosshair->setGeometry(qRound(plotArea.left()), markerY,
                                       qRound(plotArea.width()), 1);
    m_hoverPoint->setGeometry(markerX - 5, markerY - 5, 10, 10);

    const QString timeFormat = m_maxDays > 0 && m_maxDays <= 1
                                   ? QStringLiteral("yyyy-MM-dd HH:mm:ss")
                                   : QStringLiteral("yyyy-MM-dd HH:mm");
    const QString volume = point.hasVolume || point.volume > 0
                               ? QLocale().toString(point.volume)
                               : QStringLiteral("—");
    m_hoverLabel->setText(QStringLiteral("时间  %1\n价格  %2%3\n成交量  %4")
                              .arg(point.recordedAt.toLocalTime().toString(timeFormat),
                                   m_symbol, QString::number(point.price, 'f', 2), volume));
    m_hoverLabel->adjustSize();
    const QSize labelSize = m_hoverLabel->sizeHint();
    int labelX = markerX + 14;
    int labelY = markerY - labelSize.height() - 12;
    if (labelX + labelSize.width() > m_view->viewport()->width() - 6) {
        labelX = markerX - labelSize.width() - 14;
    }
    if (labelY < qRound(plotArea.top()) + 4) labelY = markerY + 12;
    labelX = qBound(6, labelX, qMax(6, m_view->viewport()->width() - labelSize.width() - 6));
    labelY = qBound(6, labelY, qMax(6, m_view->viewport()->height() - labelSize.height() - 6));
    m_hoverLabel->setGeometry(labelX, labelY, labelSize.width(), labelSize.height());

    for (QWidget *overlay : {static_cast<QWidget *>(m_verticalCrosshair),
                             static_cast<QWidget *>(m_horizontalCrosshair),
                             static_cast<QWidget *>(m_hoverPoint),
                             static_cast<QWidget *>(m_hoverLabel)}) {
        overlay->show();
        overlay->raise();
    }
}

void PriceChart::hideHover() {
    if (m_verticalCrosshair) m_verticalCrosshair->hide();
    if (m_horizontalCrosshair) m_horizontalCrosshair->hide();
    if (m_hoverPoint) m_hoverPoint->hide();
    if (m_hoverLabel) m_hoverLabel->hide();
}
