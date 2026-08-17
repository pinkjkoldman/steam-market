#pragma once

#include <QWidget>

#include "core/models/KlineBar.h"

class QChartView;
class QChart;

// 日 K 线图：蜡烛图 + 成交量柱 + MA5/10/20 均线（Qt Charts）。
class KlineChart : public QWidget {
    Q_OBJECT

public:
    explicit KlineChart(QWidget *parent = nullptr);

    void setBars(const QVector<KlineBar> &bars);

private:
    void rebuild();
    QChartView *m_view = nullptr;
    QChart *m_chart = nullptr;
    QVector<KlineBar> m_bars;
};
