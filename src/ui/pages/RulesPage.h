#pragma once

#include <QWidget>

class QComboBox;
class QTextBrowser;
class QDoubleSpinBox;
class QLabel;
class TradingRulesService;

// 交易规则页：规则库浏览 + 费用计算器。
class RulesPage : public QWidget {
    Q_OBJECT

public:
    explicit RulesPage(TradingRulesService *service, QWidget *parent = nullptr);

private:
    void reloadRules();
    void calculate();

    TradingRulesService *m_service = nullptr;
    QComboBox *m_category = nullptr;
    QTextBrowser *m_browser = nullptr;
    QComboBox *m_direction = nullptr;
    QDoubleSpinBox *m_price = nullptr;
    QLabel *m_result = nullptr;
};
