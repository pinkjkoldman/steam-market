#pragma once

#include <QWidget>

#include "core/models/PricePoint.h"

class QChartView;
class QChart;
class QEvent;
class QFrame;
class QLabel;
class QPoint;
class QXYSeries;

// 走势图：价格折线（主轴）+ 成交量面积（副轴），支持时间区间过滤。
class PriceChart : public QWidget {
    Q_OBJECT

public:
    explicit PriceChart(QWidget *parent = nullptr);

    void setPoints(const QVector<PricePoint> &points, int maxDays = 0);
    void setCurrencySymbol(const QString &symbol);
    void setSourceText(const QString &text);
    void setEmptyText(const QString &text);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void clearChart();
    void rebuild();
    void updateHover(const QPoint &position);
    void hideHover();
    QChartView *m_view = nullptr;
    QChart *m_chart = nullptr;
    QVector<PricePoint> m_points;
    QVector<PricePoint> m_displayedPoints;
    QXYSeries *m_priceSeries = nullptr;
    QFrame *m_verticalCrosshair = nullptr;
    QFrame *m_horizontalCrosshair = nullptr;
    QFrame *m_hoverPoint = nullptr;
    QLabel *m_hoverLabel = nullptr;
    int m_maxDays = 0;
    QString m_symbol = QStringLiteral("¥");
    QString m_sourceText;
};
