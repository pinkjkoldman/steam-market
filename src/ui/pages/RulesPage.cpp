#include "ui/pages/RulesPage.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPair>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCursor>
#include <QVBoxLayout>

#include "core/services/TradingRulesService.h"

RulesPage::RulesPage(TradingRulesService *service, QWidget *parent)
    : QWidget(parent), m_service(service) {
    setMinimumHeight(650);
    m_category = new QComboBox(this);
    m_category->addItem(QStringLiteral("全部"));
    const QStringList categories = m_service->categories();
    const QList<QPair<QString, QString>> labels = {
        {QStringLiteral("fee"), QStringLiteral("手续费")},
        {QStringLiteral("protection"), QStringLiteral("交易保护")},
        {QStringLiteral("wallet"), QStringLiteral("钱包")},
        {QStringLiteral("buy_order"), QStringLiteral("求购")},
        {QStringLiteral("listing"), QStringLiteral("上架")},
        {QStringLiteral("restriction"), QStringLiteral("限制")},
        {QStringLiteral("other"), QStringLiteral("其他")},
    };
    for (const QString &cat : categories) {
        QString label = cat;
        for (const auto &pair : labels) {
            if (pair.first == cat) label = pair.second;
        }
        m_category->addItem(label, cat);
    }
    m_browser = new QTextBrowser(this);
    auto *hint = new QLabel(QStringLiteral("规则内容以 Steam 官方页面为准，费率可在设置中校准"), this);

    auto *ruleGroup = new QGroupBox(QStringLiteral("Steam 市场交易规则"), this);
    auto *ruleLayout = new QVBoxLayout(ruleGroup);
    ruleLayout->setContentsMargins(12, 18, 12, 12);
    ruleLayout->setSpacing(10);
    ruleLayout->addWidget(m_category);
    ruleLayout->addWidget(m_browser, 1);

    auto *calcGroup = new QGroupBox(QStringLiteral("费用计算器（CS2 默认费率 Steam 5% + 游戏 10%）"), this);
    auto *form = new QFormLayout(calcGroup);
    form->setContentsMargins(12, 18, 12, 12);
    form->setVerticalSpacing(10);
    m_direction = new QComboBox(calcGroup);
    m_direction->addItems({QStringLiteral("卖出（输入卖家到手价）"),
                           QStringLiteral("买入（输入买家支付价）")});
    m_price = new QDoubleSpinBox(calcGroup);
    m_price->setRange(0.01, 1000000);
    m_price->setDecimals(2);
    m_price->setPrefix(QStringLiteral("¥"));
    auto *calcBtn = new QPushButton(QStringLiteral("计算"), calcGroup);
    m_result = new QLabel(QStringLiteral("—"), calcGroup);
    m_result->setWordWrap(true);
    form->addRow(QStringLiteral("方向"), m_direction);
    form->addRow(QStringLiteral("价格"), m_price);
    form->addRow(calcBtn);
    form->addRow(m_result);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("pageCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(14);
    cardLayout->addWidget(ruleGroup, 2);
    cardLayout->addWidget(calcGroup, 1);
    cardLayout->addWidget(hint);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(card, 1);

    connect(m_category, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { reloadRules(); });
    connect(calcBtn, &QPushButton::clicked, this, &RulesPage::calculate);
    reloadRules();
}

void RulesPage::reloadRules() {
    m_browser->clear();
    const QString category = m_category->currentData().toString();
    const QVector<TradingRule> rules =
        category.isEmpty() ? m_service->rules() : m_service->rulesByCategory(category);
    for (const TradingRule &r : rules) {
        m_browser->append(QStringLiteral("<h3>%1</h3>").arg(r.title.toHtmlEscaped()));
        m_browser->append(QStringLiteral("<p>%1</p>").arg(r.content.toHtmlEscaped()));
        m_browser->append(QStringLiteral("<p style='color:#9AA3B2'>来源：%1 · 更新：%2</p>")
                              .arg(r.source.toHtmlEscaped(),
                                   r.updatedAt.isValid()
                                       ? r.updatedAt.toString(QStringLiteral("yyyy-MM-dd"))
                                       : QStringLiteral("—")));
    }
    if (rules.isEmpty()) {
        m_browser->setPlainText(QStringLiteral("该分类暂无规则条目"));
    }
    m_browser->moveCursor(QTextCursor::Start);
    m_browser->ensureCursorVisible();
}

void RulesPage::calculate() {
    const QString direction =
        m_direction->currentIndex() == 0 ? QStringLiteral("sell") : QStringLiteral("buy");
    const FeeEstimate fee = m_service->estimateFee(direction, m_price->value());
    m_result->setText(QStringLiteral(
                          "买家支付：¥%1\n卖家到手：¥%2\nSteam 费：¥%3\n游戏费：¥%4\n合计费用：¥%5")
                          .arg(fee.buyerPays, 0, 'f', 2)
                          .arg(fee.sellerReceives, 0, 'f', 2)
                          .arg(fee.steamFee, 0, 'f', 2)
                          .arg(fee.gameFee, 0, 'f', 2)
                          .arg(fee.totalFee, 0, 'f', 2));
}
