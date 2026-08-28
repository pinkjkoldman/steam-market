#include "ui/pages/InventoryAssistantPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ui/widgets/LoadingOverlay.h"

void InventoryAssistantPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto *header = new QHBoxLayout();
    auto *heading = new QVBoxLayout();
    auto *title = new QLabel(QStringLiteral("库存与交易助手"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("直接读取库存、批量选择并准备价格；最终提交始终在 Steam 官方页面完成。"),
        this);
    subtitle->setObjectName(QStringLiteral("mutedText"));
    subtitle->setWordWrap(true);
    heading->addWidget(title);
    heading->addWidget(subtitle);
    m_accountBadge = new QLabel(QStringLiteral("未连接 Steam"), this);
    m_accountBadge->setObjectName(QStringLiteral("statusPill"));
    m_accountBadge->setProperty("state", QStringLiteral("disconnected"));
    header->addLayout(heading, 1);
    header->addWidget(m_accountBadge, 0, Qt::AlignTop);
    root->addLayout(header);

    buildAccountSection(root);
    buildInventorySection(root);

    m_operationStatus = new QLabel(QStringLiteral("请先连接 Steam 账户或使用公开库存。"), this);
    m_operationStatus->setObjectName(QStringLiteral("inlineStatus"));
    m_operationStatus->setWordWrap(true);
    root->addWidget(m_operationStatus);

    m_loading = new LoadingOverlay(this);
    m_loading->setText(QStringLiteral("正在同步 Steam 库存…"));
}
void InventoryAssistantPage::buildAccountSection(QVBoxLayout *root) {
    auto *box = new QGroupBox(QStringLiteral("1  连接账户"), this);
    auto *layout = new QVBoxLayout(box);
    layout->setSpacing(10);

    auto *sessionRow = new QHBoxLayout();
    m_accountDetails = new QLabel(
        QStringLiteral("私有库存请通过 Steam 官方页面登录；应用不会接收您的密码。"), box);
    m_accountDetails->setObjectName(QStringLiteral("mutedText"));
    m_accountDetails->setWordWrap(true);
    m_loginButton = new QPushButton(QStringLiteral("登录 Steam"), box);
    m_loginButton->setObjectName(QStringLiteral("primaryButton"));
    m_logoutButton = new QPushButton(QStringLiteral("退出登录"), box);
    m_logoutButton->setObjectName(QStringLiteral("secondaryButton"));
    m_logoutButton->setEnabled(false);
    m_openInventoryButton = new QPushButton(QStringLiteral("打开官方库存"), box);
    m_openInventoryButton->setObjectName(QStringLiteral("secondaryButton"));
    m_openInventoryButton->setEnabled(false);
    sessionRow->addWidget(m_accountDetails, 1);
    sessionRow->addWidget(m_openInventoryButton);
    sessionRow->addWidget(m_loginButton);
    sessionRow->addWidget(m_logoutButton);
    layout->addLayout(sessionRow);

    auto *publicRow = new QHBoxLayout();
    auto *publicLabel = new QLabel(QStringLiteral("公开库存备用方式"), box);
    m_publicSteamId = new QLineEdit(box);
    m_publicSteamId->setAccessibleName(QStringLiteral("SteamID64"));
    m_publicSteamId->setPlaceholderText(QStringLiteral("输入 17 位 SteamID64"));
    m_publicSteamId->setClearButtonEnabled(true);
    m_publicSteamId->setMaximumWidth(320);
    m_publicButton = new QPushButton(QStringLiteral("读取公开库存"), box);
    m_publicButton->setObjectName(QStringLiteral("secondaryButton"));
    publicRow->addWidget(publicLabel);
    publicRow->addWidget(m_publicSteamId);
    publicRow->addWidget(m_publicButton);
    publicRow->addStretch();
    layout->addLayout(publicRow);
    root->addWidget(box);
}

void InventoryAssistantPage::buildInventorySection(QVBoxLayout *root) {
    auto *box = new QGroupBox(QStringLiteral("2  同步并选择物品"), this);
    auto *layout = new QVBoxLayout(box);
    layout->setSpacing(10);

    auto *sourceRow = new QHBoxLayout();
    m_context = new QComboBox(box);
    m_context->setAccessibleName(QStringLiteral("库存来源"));
    m_context->addItem(QStringLiteral("Steam 社区物品 · 卡牌/表情/背景"), QStringLiteral("753:6"));
    m_context->addItem(QStringLiteral("Counter-Strike 2 库存"), QStringLiteral("730:2"));
    m_context->addItem(QStringLiteral("Dota 2 库存"), QStringLiteral("570:2"));
    m_context->addItem(QStringLiteral("Team Fortress 2 库存"), QStringLiteral("440:2"));
    m_syncButton = new QPushButton(QStringLiteral("同步库存"), box);
    m_syncButton->setObjectName(QStringLiteral("primaryButton"));
    m_syncButton->setEnabled(false);
    m_cancelButton = new QPushButton(QStringLiteral("取消"), box);
    m_cancelButton->setObjectName(QStringLiteral("secondaryButton"));
    m_cancelButton->setEnabled(false);
    m_autoSync = new QCheckBox(QStringLiteral("连接后自动同步"), box);
    m_autoSync->setChecked(true);
    auto *privacyButton = new QPushButton(QStringLiteral("库存隐私设置"), box);
    privacyButton->setObjectName(QStringLiteral("linkButton"));
    privacyButton->setProperty("action", QStringLiteral("privacy"));
    sourceRow->addWidget(m_context, 1);
    sourceRow->addWidget(m_syncButton);
    sourceRow->addWidget(m_cancelButton);
    sourceRow->addWidget(m_autoSync);
    sourceRow->addWidget(privacyButton);
    layout->addLayout(sourceRow);

    auto *filterRow = new QHBoxLayout();
    m_filter = new QLineEdit(box);
    m_filter->setAccessibleName(QStringLiteral("库存过滤"));
    m_filter->setPlaceholderText(QStringLiteral("按物品名称过滤当前库存"));
    m_filter->setClearButtonEnabled(true);
    m_cardsOnly = new QCheckBox(QStringLiteral("只看交易卡牌"), box);
    m_cardsOnly->setChecked(true);
    auto *selectAll = new QPushButton(QStringLiteral("选择当前可见"), box);
    selectAll->setObjectName(QStringLiteral("secondaryButton"));
    selectAll->setProperty("action", QStringLiteral("selectAll"));
    auto *selectDuplicates = new QPushButton(QStringLiteral("出售重复项（各留 1 件）"), box);
    selectDuplicates->setObjectName(QStringLiteral("secondaryButton"));
    selectDuplicates->setProperty("action", QStringLiteral("duplicates"));
    auto *clear = new QPushButton(QStringLiteral("清空选择"), box);
    clear->setObjectName(QStringLiteral("linkButton"));
    clear->setProperty("action", QStringLiteral("clear"));
    filterRow->addWidget(m_filter, 1);
    filterRow->addWidget(m_cardsOnly);
    filterRow->addWidget(selectAll);
    filterRow->addWidget(selectDuplicates);
    filterRow->addWidget(clear);
    layout->addLayout(filterRow);

    auto *batchRow = new QHBoxLayout();
    auto *batchLabel = new QLabel(QStringLiteral("批量上架"), box);
    m_price = new QDoubleSpinBox(box);
    m_price->setAccessibleName(QStringLiteral("批量上架买家支付单价"));
    m_price->setRange(0.03, 1000000.0);
    m_price->setDecimals(2);
    m_price->setPrefix(QStringLiteral("¥"));
    m_price->setValue(0.10);
    m_summary = new QLabel(QStringLiteral("登录后勾选多个物品即可批量上架"), box);
    m_summary->setObjectName(QStringLiteral("mutedText"));
    m_summary->setWordWrap(true);
    m_handoffButton = new QPushButton(QStringLiteral("批量上架已选物品"), box);
    m_handoffButton->setObjectName(QStringLiteral("batchListingVisibleAction"));
    m_handoffButton->setEnabled(false);
    m_handoffButton->setToolTip(QStringLiteral("登录 Steam 并勾选至少一个可出售物品后启用"));
    batchRow->addWidget(batchLabel);
    batchRow->addWidget(new QLabel(QStringLiteral("每件买家支付"), box));
    batchRow->addWidget(m_price);
    batchRow->addSpacing(8);
    batchRow->addWidget(m_summary, 1);
    batchRow->addWidget(m_handoffButton);
    layout->addLayout(batchRow);

    auto *singleRow = new QHBoxLayout();
    auto *singleLabel = new QLabel(QStringLiteral("当前行单品操作"), box);
    m_singleListingButton = new QPushButton(QStringLiteral("上架到 Steam 市场"), box);
    m_singleListingButton->setObjectName(QStringLiteral("secondaryButton"));
    m_singleListingButton->setEnabled(false);
    m_tradeButton = new QPushButton(QStringLiteral("选择交易用户"), box);
    m_tradeButton->setObjectName(QStringLiteral("secondaryButton"));
    m_tradeButton->setEnabled(false);
    auto *singleHint = new QLabel(
        QStringLiteral("选中一行后操作；最终提交与确认始终在 Steam 官方页面完成"),
        box);
    singleHint->setObjectName(QStringLiteral("mutedText"));
    singleHint->setWordWrap(true);
    singleRow->addWidget(singleLabel);
    singleRow->addWidget(m_singleListingButton);
    singleRow->addWidget(m_tradeButton);
    singleRow->addWidget(singleHint, 1);
    layout->addLayout(singleRow);

    m_table = new QTableWidget(0, 8, box);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("选择"), QStringLiteral("物品"), QStringLiteral("类型"),
         QStringLiteral("持有数量"), QStringLiteral("交易状态"),
         QStringLiteral("市场出售"), QStringLiteral("计划出售"),
         QStringLiteral("历史趋势")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setAccessibleName(QStringLiteral("库存物品列表"));
    m_table->setToolTip(QStringLiteral("双击查看历史；右键可上架单品或选择交易用户"));
    m_table->setMinimumHeight(250);
    layout->addWidget(m_table, 1);

    m_selectionStatus = new QLabel(QStringLiteral("尚未加载库存"), box);
    m_selectionStatus->setObjectName(QStringLiteral("mutedText"));
    layout->addWidget(m_selectionStatus);
    root->addWidget(box, 1);
}
