#pragma once

#include <QWidget>

class QComboBox;
class QTableView;
class QStandardItemModel;
class QLabel;
class RankingService;

// 排行榜页：成交量/涨幅/跌幅榜。
class RankingPage : public QWidget {
    Q_OBJECT

public:
    explicit RankingPage(RankingService *service, QWidget *parent = nullptr);

signals:
    void itemSelected(const QString &marketHashName, int appid);

private:
    void reload();

    RankingService *m_service = nullptr;
    QComboBox *m_by = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_status = nullptr;
};
