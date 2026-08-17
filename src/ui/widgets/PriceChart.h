#pragma once

#include <QWidget>

#include "core/models/PricePoint.h"

class QChartView;
class QChart;
class QLineSeries;
class QBarSeries;

// 走势图：价格折线（主轴）+ 销量柱状（副轴），支持时间区间过滤。
class PriceChart : public QWidget {
    Q_OBJECT

public:
    explicit PriceChart(QWidget *parent = nullptr);

    void setPoints(const QVector<PricePoint> &points, int maxDays = 0);
    void setCurrencySymbol(const QString &symbol);
    void setEmptyText(const QString &text);

private:
    void rebuild();
    QChartView *m_view = nullptr;
    QChart *m_chart = nullptr;
    QVector<PricePoint> m_points;
    int m_maxDays = 0;
    QString m_symbol = QStringLiteral("¥");
};
