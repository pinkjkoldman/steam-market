#include "ui/pages/RankingPage.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "core/services/RankingService.h"
#include "ui/widgets/LoadingOverlay.h"
#include "utils/Currency.h"
#include "utils/CurrencyProvider.h"

RankingPage::RankingPage(RankingService *service, QWidget *parent)
    : QWidget(parent), m_service(service) {
    m_by = new QComboBox(this);
    m_by->addItems({QStringLiteral("成交量榜"), QStringLiteral("涨幅榜"), QStringLiteral("跌幅榜")});
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    auto *hint = new QLabel(
        QStringLiteral("数据来自自选与已浏览物品的本地快照，浏览越多榜单越完整"), this);
    auto *topRow = new QHBoxLayout();
    topRow->addWidget(m_by);
    topRow->addWidget(refreshBtn);
    topRow->addStretch();

    m_table = new QTableView(this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(
        {QStringLiteral("排名"), QStringLiteral("物品名称"), QStringLiteral("当前价"),
         QStringLiteral("指标值")});
    m_table->setModel(m_model);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    m_status = new QLabel(this);

    m_card = new QFrame(this);
    m_card->setObjectName(QStringLiteral("pageCard"));
    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);
    cardLayout->addLayout(topRow);
    cardLayout->addWidget(hint);
    cardLayout->addWidget(m_table, 1);
    cardLayout->addWidget(m_status);

    m_emptyState = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(8);
    m_emptyIcon = new QLabel(QStringLiteral("🏆"), m_emptyState);
    m_emptyIcon->setObjectName(QStringLiteral("emptyStateIcon"));
    m_emptyIcon->setAlignment(Qt::AlignCenter);
    m_emptyTitle = new QLabel(QStringLiteral("暂无榜单数据"), m_emptyState);
    m_emptyTitle->setObjectName(QStringLiteral("emptyStateTitle"));
    m_emptyTitle->setAlignment(Qt::AlignCenter);
    m_emptySubtitle = new QLabel(QStringLiteral("搜索并浏览更多物品后，榜单将自动生成"), m_emptyState);
    m_emptySubtitle->setObjectName(QStringLiteral("emptyStateSubtitle"));
    m_emptySubtitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(m_emptyIcon);
    emptyLayout->addWidget(m_emptyTitle);
    emptyLayout->addWidget(m_emptySubtitle);

    auto *viewStack = new QStackedWidget(this);
    viewStack->addWidget(m_card);
    viewStack->addWidget(m_emptyState);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(viewStack, 1);

    m_loading = new LoadingOverlay(this);
    m_loading->setText(QStringLiteral("正在生成排行榜…"));

    auto updateRanking = [this]() {
        m_loading->show();
        reload();
    };
    connect(m_by, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, updateRanking](int) { updateRanking(); });
    connect(refreshBtn, &QPushButton::clicked, this, [this, updateRanking]() { updateRanking(); });
    connect(m_table, &QTableView::doubleClicked, this, [this]() {
        const QModelIndex idx = m_table->currentIndex();
        if (idx.isValid()) {
            emit itemSelected(m_model->item(idx.row(), 1)->data(Qt::UserRole).toString(),
                              m_model->item(idx.row(), 1)->data(Qt::UserRole + 1).toInt());
        }
    });
    reload();
}

void RankingPage::reload() {
    m_model->removeRows(0, m_model->rowCount());
    RankingService::By by = RankingService::By::kVolume;
    if (m_by->currentIndex() == 1) by = RankingService::By::kGain;
    if (m_by->currentIndex() == 2) by = RankingService::By::kLoss;
    const QVector<TopItem> items = m_service->top(by, 50);
    const QString symbol = Currency::displaySymbol(CurrencyProvider::code());
    for (const TopItem &it : items) {
        auto *nameItem = new QStandardItem(it.name);
        nameItem->setData(it.marketHashName, Qt::UserRole);
        nameItem->setData(it.appid, Qt::UserRole + 1);
        const QString valueText = by == RankingService::By::kVolume
                                      ? QString::number(it.value)
                                      : QStringLiteral("%1%2%")
                                            .arg(it.value >= 0 ? QStringLiteral("+") : QString())
                                            .arg(it.value, 0, 'f', 2);
        m_model->appendRow({new QStandardItem(QString::number(it.rank)), nameItem,
                            new QStandardItem(it.price > 0
                                                  ? symbol + QString::number(it.price, 'f', 2)
                                                  : QStringLiteral("—")),
                            new QStandardItem(valueText)});
    }
    m_status->setText(items.isEmpty()
                          ? QStringLiteral("数据积累中：搜索/收藏更多物品后自动生成榜单")
                          : QStringLiteral("共 %1 项").arg(items.size()));
    updateEmptyState(items.isEmpty());
    m_loading->hide();
}

void RankingPage::updateEmptyState(bool empty) {
    auto *stack = qobject_cast<QStackedWidget *>(layout()->itemAt(0)->widget());
    if (stack) stack->setCurrentIndex(empty ? 1 : 0);
}
