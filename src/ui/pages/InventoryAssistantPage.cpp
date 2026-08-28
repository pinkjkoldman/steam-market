#include "ui/pages/InventoryAssistantPage.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

#include "core/services/InventoryService.h"
#include "core/services/MultiSellHandoffService.h"
#include "core/services/PricingDraftService.h"
#include "core/services/SteamSessionService.h"
#include "core/services/TradeHandoffService.h"
#include "ui/widgets/WorkbenchTheme.h"
#include "ui/widgets/LoadingOverlay.h"

namespace {
QString categoryText(const QString &category) {
    if (category == QLatin1String("trading_card")) return QStringLiteral("卡牌");
    if (category == QLatin1String("emoticon")) return QStringLiteral("表情");
    if (category == QLatin1String("profile_background")) return QStringLiteral("背景");
    return QStringLiteral("其他");
}

QPushButton *actionButton(QWidget *root, const QString &action) {
    const auto buttons = root->findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->property("action").toString() == action) return button;
    }
    return nullptr;
}
}

InventoryAssistantPage::InventoryAssistantPage(
    SteamSessionService *session, InventoryService *inventory,
    PricingDraftService *pricing, MultiSellHandoffService *handoff, QWidget *parent)
    : QWidget(parent), m_session(session), m_inventory(inventory), m_pricing(pricing),
      m_handoff(handoff) {
    setProperty("workbench", true);
    setStyleSheet(WorkbenchTheme::styleSheet());
    buildUi();
    wireSignals();
}

void InventoryAssistantPage::wireSignals() {
    connect(m_loginButton, &QPushButton::clicked, m_session, &SteamSessionService::showLogin);
    connect(m_logoutButton, &QPushButton::clicked, m_session, &SteamSessionService::logout);
    connect(m_publicButton, &QPushButton::clicked, this, [this]() {
        m_session->usePublicInventory(m_publicSteamId->text());
    });
    connect(m_openInventoryButton, &QPushButton::clicked, this, [this]() {
        if (!m_session->steamId().isEmpty()) {
            m_session->openOfficialUrl(
                QUrl(QStringLiteral("https://steamcommunity.com/profiles/%1/inventory/")
                         .arg(m_session->steamId())));
        }
    });
    if (auto *privacy = actionButton(this, QStringLiteral("privacy"))) {
        connect(privacy, &QPushButton::clicked, this, [this]() {
            m_session->openOfficialUrl(
                QUrl(QStringLiteral("https://steamcommunity.com/my/edit/settings")));
        });
    }
    connect(m_syncButton, &QPushButton::clicked, this, &InventoryAssistantPage::startSync);
    connect(m_cancelButton, &QPushButton::clicked, m_inventory, &InventoryService::cancel);
    connect(m_context, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        const bool communityItems = currentContext().first == 753;
        m_cardsOnly->setEnabled(communityItems);
        m_cardsOnly->setChecked(communityItems);
        m_lastAutoSyncKey.clear();
        resetHandoff();
        loadCachedInventory();
    });
    connect(m_filter, &QLineEdit::textChanged, this, &InventoryAssistantPage::applyFilter);
    connect(m_cardsOnly, &QCheckBox::toggled, this, &InventoryAssistantPage::applyFilter);
    connect(m_price, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        resetHandoff();
        updateSummary();
    });
    connect(m_handoffButton, &QPushButton::clicked, this, [this]() {
        if (m_nextBatchIndex < m_batches.size()) openNextBatch();
        else prepareHandoff();
    });
    if (auto *button = actionButton(this, QStringLiteral("selectAll"))) {
        connect(button, &QPushButton::clicked, this, [this]() { selectVisible(false); });
    }
    if (auto *button = actionButton(this, QStringLiteral("duplicates"))) {
        connect(button, &QPushButton::clicked, this, [this]() { selectVisible(true); });
    }
    if (auto *button = actionButton(this, QStringLiteral("clear"))) {
        connect(button, &QPushButton::clicked, this, &InventoryAssistantPage::clearSelection);
    }
    connect(m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (item && item->column() == 0) {
            resetHandoff();
            updateSummary();
        }
    });
    connect(m_singleListingButton, &QPushButton::clicked, this, [this]() {
        openSingleListingForRow(m_table->currentRow());
    });
    connect(m_tradeButton, &QPushButton::clicked, this, [this]() {
        openTradeForRow(m_table->currentRow());
    });
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { requestHistoryForRow(row); });
    connect(m_table, &QTableWidget::currentCellChanged, this,
            [this](int, int, int, int) { updateSingleItemActions(); });
    connect(m_table, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint &position) {
        const QModelIndex index = m_table->indexAt(position);
        if (!index.isValid()) return;
        m_table->selectRow(index.row());
        QMenu menu(m_table);
        QAction *historyAction = menu.addAction(QStringLiteral("查看历史趋势"));
        menu.addSeparator();
        QAction *listingAction = menu.addAction(QStringLiteral("上架此物品到 Steam 市场"));
        QAction *tradeAction = menu.addAction(QStringLiteral("与 Steam 用户交易此物品"));
        const int groupIndex = groupIndexForRow(index.row());
        const bool validGroup = groupIndex >= 0 && groupIndex < m_groups.size();
        historyAction->setEnabled(groupIndex >= 0 && groupIndex < m_groups.size()
                                  && !m_groups.at(groupIndex).marketHashName.trimmed().isEmpty());
        listingAction->setEnabled(validGroup && m_authenticated
                                  && m_groups.at(groupIndex).marketable);
        tradeAction->setEnabled(validGroup && m_authenticated
                                && m_groups.at(groupIndex).tradable);
        QAction *selectedAction = menu.exec(m_table->viewport()->mapToGlobal(position));
        if (selectedAction == historyAction) {
            requestHistoryForRow(index.row());
        } else if (selectedAction == listingAction) {
            openSingleListingForRow(index.row());
        } else if (selectedAction == tradeAction) {
            openTradeForRow(index.row());
        }
    });

    connect(m_session, &SteamSessionService::sessionChanged, this,
            &InventoryAssistantPage::onSessionChanged);
    connect(m_session, &SteamSessionService::sessionError, this,
            [this](const QString &message) { setOperationStatus(message, true); });
    connect(m_inventory, &InventoryService::syncProgress, this,
            [this](int pages, int assets) {
        m_syncButton->setText(pages == 0 ? QStringLiteral("连接中…")
                                         : QStringLiteral("同步中 · %1 页").arg(pages));
        setOperationStatus(QStringLiteral("正在同步：%1 页，已读取 %2 个库存资产…")
                               .arg(pages)
                               .arg(assets));
    });
    connect(m_inventory, &InventoryService::syncCompleted, this,
            [this](const QVector<InventoryGroup> &groups) {
        m_syncButton->setText(QStringLiteral("同步库存"));
        m_syncButton->setEnabled(true);
        m_cancelButton->setEnabled(false);
        m_loading->hide();
        populateGroups(groups);
        const int marketableCount = static_cast<int>(std::count_if(
            groups.cbegin(), groups.cend(),
            [](const InventoryGroup &group) { return group.marketable; }));
        setOperationStatus(groups.isEmpty()
                               ? QStringLiteral("同步成功，但此库存分类没有物品。可切换库存来源后重试。")
                               : QStringLiteral("同步完成：共 %1 组物品，其中 %2 组可在市场出售。")
                                     .arg(groups.size()).arg(marketableCount));
    });
    connect(m_inventory, &InventoryService::syncFailed, this,
            [this](const QString &message) {
        m_syncButton->setText(QStringLiteral("重新同步"));
        m_syncButton->setEnabled(m_session->hasSession());
        m_cancelButton->setEnabled(false);
        m_loading->hide();
        loadCachedInventory();
        setOperationStatus(QStringLiteral("同步失败：%1  已保留上次成功的库存。")
                               .arg(message), true);
    });
}

void InventoryAssistantPage::onSessionChanged(bool authenticated, const QString &steamId,
                                              const QString &displayName) {
    m_authenticated = authenticated;
    const bool connected = !steamId.isEmpty();
    m_loginButton->setEnabled(!authenticated);
    m_logoutButton->setEnabled(connected);
    m_openInventoryButton->setEnabled(connected);
    m_syncButton->setEnabled(connected && !m_inventory->isSyncing());
    updateSingleItemActions();
    updateSummary();
    if (!connected) {
        m_accountBadge->setText(QStringLiteral("未连接 Steam"));
        m_accountBadge->setProperty("state", QStringLiteral("disconnected"));
        m_accountBadge->style()->unpolish(m_accountBadge);
        m_accountBadge->style()->polish(m_accountBadge);
        m_accountDetails->setText(
            QStringLiteral("私有库存请通过 Steam 官方页面登录；应用不会接收您的密码。"));
        populateGroups({});
        setOperationStatus(QStringLiteral("请先连接 Steam 账户或使用公开库存。"));
        return;
    }

    m_accountBadge->setText(authenticated ? QStringLiteral("已安全登录")
                                          : QStringLiteral("公开库存模式"));
    m_accountBadge->setProperty("state", authenticated ? QStringLiteral("authenticated")
                                                       : QStringLiteral("public"));
    m_accountBadge->style()->unpolish(m_accountBadge);
    m_accountBadge->style()->polish(m_accountBadge);
    m_accountDetails->setText(
        authenticated ? QStringLiteral("%1 · SteamID64 %2 · 登录 Cookie 仅保存在内存中")
                            .arg(displayName, steamId)
                      : QStringLiteral("SteamID64 %1 · 只能读取公开可见库存").arg(steamId));
    loadCachedInventory();
    const QString autoSyncKey = steamId + QLatin1Char(':') + m_context->currentData().toString();
    if (m_autoSync->isChecked() && m_lastAutoSyncKey != autoSyncKey) {
        m_lastAutoSyncKey = autoSyncKey;
        QTimer::singleShot(0, this, &InventoryAssistantPage::startSync);
    }
}

QPair<int, QString> InventoryAssistantPage::currentContext() const {
    const QStringList parts = m_context->currentData().toString().split(QLatin1Char(':'));
    return {parts.value(0).toInt(), parts.value(1)};
}

void InventoryAssistantPage::startSync() {
    if (!m_session->hasSession()) {
        setOperationStatus(QStringLiteral("请先登录 Steam，或输入公开库存的 SteamID64。"), true);
        return;
    }
    if (m_inventory->isSyncing()) return;
    const auto context = currentContext();
    resetHandoff();
    m_syncButton->setText(QStringLiteral("连接中…"));
    m_syncButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_loading->show();
    setOperationStatus(QStringLiteral("正在连接 Steam 库存…"));
    m_inventory->sync(m_session->steamId(), context.first, context.second);
}

void InventoryAssistantPage::loadCachedInventory() {
    if (!m_session->hasSession() || m_inventory->isSyncing()) return;
    const auto context = currentContext();
    const QVector<InventoryGroup> cached =
        m_inventory->groups(m_session->steamId(), context.first, context.second);
    populateGroups(cached);
    if (!cached.isEmpty()) {
        setOperationStatus(QStringLiteral("已显示上次成功同步的 %1 组物品；可点击“同步库存”刷新。")
                               .arg(cached.size()));
    }
}

void InventoryAssistantPage::populateGroups(const QVector<InventoryGroup> &groups) {
    m_groups = groups;
    m_table->blockSignals(true);
    m_table->setRowCount(groups.size());
    for (int row = 0; row < groups.size(); ++row) {
        const InventoryGroup &group = groups.at(row);
        auto *check = new QTableWidgetItem();
        check->setCheckState(Qt::Unchecked);
        check->setData(Qt::UserRole, row);
        check->setToolTip(group.marketable
                              ? QStringLiteral("勾选后可加入出售计划")
                              : QStringLiteral("Steam 标记为不可在市场出售，不能选择"));
        if (!group.marketable) {
            check->setFlags(check->flags() & ~Qt::ItemIsUserCheckable);
        }
        m_table->setItem(row, 0, check);
        auto *name = new QTableWidgetItem(group.displayName);
        name->setToolTip(group.marketHashName.trimmed().isEmpty()
                             ? QStringLiteral("此物品没有 Steam 市场页面")
                             : QStringLiteral("市场名称：%1\n双击或右键查看历史趋势")
                                   .arg(group.marketHashName));
        m_table->setItem(row, 1, name);
        m_table->setItem(row, 2, new QTableWidgetItem(categoryText(group.category)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(group.inventoryQuantity)));

        auto *tradable = new QTableWidgetItem(group.tradable
                                                   ? QStringLiteral("可交易")
                                                   : QStringLiteral("不可交易"));
        tradable->setToolTip(group.tradable
                                 ? QStringLiteral("可与其他 Steam 用户进行物品交易")
                                 : QStringLiteral("Steam 当前标记为不可交易，可能受物品规则或交易锁定限制"));
        tradable->setForeground(QColor(group.tradable ? QStringLiteral("#66D9A8")
                                                      : QStringLiteral("#F3B95F")));
        tradable->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 4, tradable);

        auto *marketable = new QTableWidgetItem(group.marketable
                                                     ? QStringLiteral("可出售")
                                                     : QStringLiteral("不可出售"));
        marketable->setToolTip(group.marketable
                                   ? QStringLiteral("可进入出售计划，并交接到 Steam 官方市场确认")
                                   : QStringLiteral("Steam 当前标记为不可在社区市场出售"));
        marketable->setForeground(QColor(group.marketable ? QStringLiteral("#66D9A8")
                                                          : QStringLiteral("#F17878")));
        marketable->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 5, marketable);

        auto *quantity = new QSpinBox(m_table);
        quantity->setRange(1, qMax(1, qMin(group.inventoryQuantity, group.assetIds.size())));
        quantity->setValue(quantity->maximum());
        quantity->setEnabled(false);
        connect(quantity, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
            resetHandoff();
            updateSummary();
        });
        m_table->setCellWidget(row, 6, quantity);

        auto *history = new QTableWidgetItem(group.marketHashName.trimmed().isEmpty()
                                                  ? QStringLiteral("无市场页")
                                                  : QStringLiteral("双击查看"));
        history->setTextAlignment(Qt::AlignCenter);
        history->setForeground(QColor(group.marketHashName.trimmed().isEmpty()
                                          ? QStringLiteral("#718397")
                                          : QStringLiteral("#8CD2FF")));
        history->setToolTip(group.marketHashName.trimmed().isEmpty()
                                ? QStringLiteral("Steam 未提供市场名称，无法查询历史趋势")
                                : QStringLiteral("双击当前行，或右键选择“查看历史趋势”"));
        m_table->setItem(row, 7, history);
        m_table->setRowHeight(row, 36);
    }
    m_table->blockSignals(false);
    resetHandoff();
    applyFilter();
    updateSummary();
    updateSingleItemActions();
}

int InventoryAssistantPage::groupIndexForRow(int row) const {
    if (row < 0 || row >= m_table->rowCount()) return -1;
    const QTableWidgetItem *item = m_table->item(row, 0);
    if (!item) return -1;
    const int groupIndex = item->data(Qt::UserRole).toInt();
    return groupIndex >= 0 && groupIndex < m_groups.size() ? groupIndex : -1;
}

void InventoryAssistantPage::updateSingleItemActions() {
    const int groupIndex = groupIndexForRow(m_table ? m_table->currentRow() : -1);
    const bool valid = groupIndex >= 0;
    if (m_singleListingButton) {
        m_singleListingButton->setEnabled(valid && m_authenticated
                                          && m_groups.at(groupIndex).marketable);
    }
    if (m_tradeButton) {
        m_tradeButton->setEnabled(valid && m_authenticated
                                  && m_groups.at(groupIndex).tradable);
    }
}

void InventoryAssistantPage::openSingleListingForRow(int row) {
    const int groupIndex = groupIndexForRow(row);
    if (groupIndex < 0) {
        setOperationStatus(QStringLiteral("请先在库存表中选中一个物品。"), true);
        return;
    }
    if (!m_authenticated) {
        setOperationStatus(QStringLiteral("单品上架需要先完成 Steam 官方登录。"), true);
        return;
    }
    const InventoryGroup &group = m_groups.at(groupIndex);
    TradeHandoffService handoff;
    QString error;
    const QUrl url = handoff.createSingleListingUrl(group, m_session->steamId(), &error);
    if (url.isEmpty()) {
        setOperationStatus(error, true);
        return;
    }
    const QString prompt = QStringLiteral(
        "即将在 Steam 官方页面打开“%1”。\n\n"
        "应用不会自动填写价格、提交上架或确认 Steam Guard，请您在官方页面核对。")
                               .arg(group.displayName);
    if (QMessageBox::question(this, QStringLiteral("单品上架"), prompt)
        != QMessageBox::Yes) {
        return;
    }
    m_session->openOfficialUrl(url);
    setOperationStatus(QStringLiteral("已打开 Steam 官方上架页面：%1。请手动核对价格并确认。")
                           .arg(group.displayName));
}

void InventoryAssistantPage::openTradeForRow(int row) {
    const int groupIndex = groupIndexForRow(row);
    if (groupIndex < 0) {
        setOperationStatus(QStringLiteral("请先在库存表中选中一个物品。"), true);
        return;
    }
    if (!m_authenticated) {
        setOperationStatus(QStringLiteral("用户交易需要先完成 Steam 官方登录。"), true);
        return;
    }
    const InventoryGroup &group = m_groups.at(groupIndex);
    if (!group.tradable) {
        setOperationStatus(QStringLiteral("Steam 当前将该物品标记为不可交易。"), true);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("选择 Steam 交易用户"));
    dialog.setMinimumWidth(560);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *item = new QLabel(group.displayName, &dialog);
    item->setWordWrap(true);
    auto *partner = new QLineEdit(&dialog);
    partner->setPlaceholderText(
        QStringLiteral("留空则在 Steam 选择好友，或输入 17 位 SteamID64 / 官方交易链接"));
    partner->setClearButtonEnabled(true);
    form->addRow(QStringLiteral("交易物品"), item);
    form->addRow(QStringLiteral("交易对象"), partner);
    layout->addLayout(form);
    auto *notice = new QLabel(
        QStringLiteral("应用只负责打开通过校验的 Steam 官方报价页面；交易 token 不会保存或写入日志。"
                       "进入 Steam 后请手动选择该物品并完成最终确认。"),
        &dialog);
    notice->setObjectName(QStringLiteral("mutedText"));
    notice->setWordWrap(true);
    layout->addWidget(notice);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel,
                                         &dialog);
    buttons->button(QDialogButtonBox::Open)->setText(QStringLiteral("打开 Steam 交易页"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) return;

    TradeHandoffService handoff;
    QString error;
    const QUrl url = handoff.createTradeOfferUrl(partner->text(), &error);
    if (url.isEmpty()) {
        setOperationStatus(error, true);
        return;
    }
    m_session->openOfficialUrl(url);
    setOperationStatus(QStringLiteral("已打开 Steam 官方交易报价页面。请选择“%1”并手动确认对方身份。")
                           .arg(group.displayName));
}

void InventoryAssistantPage::requestHistoryForRow(int row) {
    QTableWidgetItem *check = m_table->item(row, 0);
    if (!check) return;
    const int groupIndex = check->data(Qt::UserRole).toInt();
    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    const InventoryGroup &group = m_groups.at(groupIndex);
    if (group.marketHashName.trimmed().isEmpty()) {
        setOperationStatus(QStringLiteral("此物品缺少市场名称，暂时无法打开历史趋势。"), true);
        return;
    }
    emit historyRequested(group.marketHashName, group.appid);
}

void InventoryAssistantPage::applyFilter() {
    const QString query = m_filter->text().trimmed();
    int visibleCount = 0;
    int visibleMarketableCount = 0;
    for (int row = 0; row < m_groups.size(); ++row) {
        const InventoryGroup &group = m_groups.at(row);
        const bool queryMatches = query.isEmpty()
                                  || group.displayName.contains(query, Qt::CaseInsensitive)
                                  || group.marketHashName.contains(query, Qt::CaseInsensitive);
        const bool categoryMatches = !m_cardsOnly->isChecked()
                                     || group.category == QLatin1String("trading_card");
        const bool visible = queryMatches && categoryMatches;
        m_table->setRowHidden(row, !visible);
        if (visible) {
            ++visibleCount;
            if (group.marketable) ++visibleMarketableCount;
        }
    }
    m_selectionStatus->setText(
        QStringLiteral("显示 %1 / %2 组物品 · 当前可见中 %3 组可出售")
            .arg(visibleCount).arg(m_groups.size()).arg(visibleMarketableCount));
}

void InventoryAssistantPage::selectVisible(bool duplicatesOnly) {
    m_table->blockSignals(true);
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->isRowHidden(row)) continue;
        const InventoryGroup &group = m_groups.at(row);
        const bool selected = group.marketable
                              && (!duplicatesOnly || group.inventoryQuantity > 1);
        m_table->item(row, 0)->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 6))) {
            quantity->setEnabled(selected);
            quantity->setValue(duplicatesOnly && selected
                                   ? qMin(quantity->maximum(), group.inventoryQuantity - 1)
                                   : quantity->maximum());
        }
    }
    m_table->blockSignals(false);
    resetHandoff();
    updateSummary();
}

void InventoryAssistantPage::clearSelection() {
    m_table->blockSignals(true);
    for (int row = 0; row < m_table->rowCount(); ++row) {
        m_table->item(row, 0)->setCheckState(Qt::Unchecked);
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 6))) {
            quantity->setEnabled(false);
        }
    }
    m_table->blockSignals(false);
    resetHandoff();
    updateSummary();
}

QVector<InventoryGroup> InventoryAssistantPage::selectedGroups() const {
    QVector<InventoryGroup> selected;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QTableWidgetItem *check = m_table->item(row, 0);
        if (!check || check->checkState() != Qt::Checked) continue;
        const int groupIndex = check->data(Qt::UserRole).toInt();
        if (groupIndex < 0 || groupIndex >= m_groups.size()) continue;
        InventoryGroup group = m_groups.at(groupIndex);
        if (!group.marketable) continue;
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 6))) {
            group.selectedQuantity = quantity->value();
        }
        selected.append(group);
    }
    return selected;
}

void InventoryAssistantPage::updateSummary() {
    const QVector<InventoryGroup> selected = selectedGroups();
    qint64 itemCount = 0;
    for (const InventoryGroup &group : selected) itemCount += group.selectedQuantity;
    const qint64 buyerPaysMinor = qRound64(m_price->value() * 100.0);
    const qint64 receiveMinor = (buyerPaysMinor * 10000) / 11500;
    const qint64 totalBuyer = buyerPaysMinor * itemCount;
    const qint64 totalReceive = receiveMinor * itemCount;
    const qint64 totalFee = totalBuyer - totalReceive;
    if (!m_authenticated) {
        m_summary->setText(QStringLiteral("登录 Steam 后可批量上架 · 当前已选 %1 组 / %2 件")
                               .arg(selected.size()).arg(itemCount));
        m_handoffButton->setToolTip(QStringLiteral("批量上架需要先完成 Steam 官方登录"));
    } else if (selected.isEmpty()) {
        m_summary->setText(QStringLiteral("请在下方表格第一列勾选多个可出售物品"));
        m_handoffButton->setToolTip(QStringLiteral("请先勾选至少一个可出售物品"));
    } else {
        m_summary->setText(
            QStringLiteral("%1 组 / %2 件 · 买家合计 ¥%3 · 预计实收 ¥%4 · 手续费 ¥%5")
                .arg(selected.size())
                .arg(itemCount)
                .arg(totalBuyer / 100.0, 0, 'f', 2)
                .arg(totalReceive / 100.0, 0, 'f', 2)
                .arg(totalFee / 100.0, 0, 'f', 2));
        m_handoffButton->setToolTip(
            QStringLiteral("检查已选物品并分批打开 Steam 官方批量出售页面"));
    }
    m_handoffButton->setEnabled(!selected.isEmpty() && m_authenticated);
    if (m_batches.isEmpty()) {
        m_handoffButton->setText(itemCount > 0
                                     ? QStringLiteral("批量上架已选（%1 件）").arg(itemCount)
                                     : QStringLiteral("批量上架已选物品"));
    }
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 6))) {
            quantity->setEnabled(m_groups.at(row).marketable
                                 && m_table->item(row, 0)->checkState() == Qt::Checked);
        }
    }
}

void InventoryAssistantPage::resetHandoff() {
    m_batches.clear();
    m_nextBatchIndex = 0;
    if (m_handoffButton) {
        m_handoffButton->setText(QStringLiteral("批量上架已选物品"));
    }
}

void InventoryAssistantPage::prepareHandoff() {
    if (!m_authenticated) {
        setOperationStatus(QStringLiteral("批量上架需要先完成 Steam 官方登录。"), true);
        return;
    }
    QString error;
    const QVector<ListingDraftLine> lines = m_pricing->createFixedPriceDraft(
        selectedGroups(), qRound64(m_price->value() * 100.0), &error);
    if (!error.isEmpty()) {
        setOperationStatus(error, true);
        return;
    }
    m_batches = m_handoff->createBatches(lines, &error);
    if (!error.isEmpty()) {
        setOperationStatus(error, true);
        return;
    }
    const QString prompt = QStringLiteral(
        "将分 %1 批打开 Steam 官方批量出售页面。应用不会填写价格或自动提交；请在官方页面再次核对物品、数量与价格。")
                               .arg(m_batches.size());
    if (QMessageBox::question(this, QStringLiteral("交接到 Steam"), prompt)
        != QMessageBox::Yes) {
        resetHandoff();
        return;
    }
    m_nextBatchIndex = 0;
    openNextBatch();
}

void InventoryAssistantPage::openNextBatch() {
    if (m_nextBatchIndex >= m_batches.size()) {
        resetHandoff();
        updateSummary();
        return;
    }
    const HandoffBatch batch = m_batches.at(m_nextBatchIndex++);
    m_session->openOfficialUrl(batch.officialUrl);
    setOperationStatus(QStringLiteral("已打开第 %1/%2 批（%3 组）。请在 Steam 官方页面核对并手动确认。")
                           .arg(batch.batchNo)
                           .arg(m_batches.size())
                           .arg(batch.groupCount));
    m_handoffButton->setText(m_nextBatchIndex < m_batches.size()
                                 ? QStringLiteral("打开下一批 Steam 页面")
                                 : QStringLiteral("完成交接并重新检查"));
}

void InventoryAssistantPage::setOperationStatus(const QString &text, bool isError) {
    m_operationStatus->setText(text);
    m_operationStatus->setProperty("error", isError);
    m_operationStatus->style()->unpolish(m_operationStatus);
    m_operationStatus->style()->polish(m_operationStatus);
}
