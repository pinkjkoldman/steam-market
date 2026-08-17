#include "ui/pages/InventoryAssistantPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>

#include "core/services/InventoryService.h"
#include "core/services/MultiSellHandoffService.h"
#include "core/services/PricingDraftService.h"
#include "core/services/SteamSessionService.h"

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

    connect(m_session, &SteamSessionService::sessionChanged, this,
            &InventoryAssistantPage::onSessionChanged);
    connect(m_session, &SteamSessionService::sessionError, this,
            [this](const QString &message) { setOperationStatus(message, true); });
    connect(m_inventory, &InventoryService::syncProgress, this,
            [this](int pages, int assets) {
        setOperationStatus(QStringLiteral("正在同步：%1 页，已读取 %2 个库存资产…")
                               .arg(pages)
                               .arg(assets));
    });
    connect(m_inventory, &InventoryService::syncCompleted, this,
            [this](const QVector<InventoryGroup> &groups) {
        m_syncButton->setEnabled(true);
        m_cancelButton->setEnabled(false);
        populateGroups(groups);
        setOperationStatus(groups.isEmpty()
                               ? QStringLiteral("同步成功，但此分类没有可在市场出售的物品。可切换库存来源后重试。")
                               : QStringLiteral("同步完成：已加载 %1 组可出售物品。").arg(groups.size()));
    });
    connect(m_inventory, &InventoryService::syncFailed, this,
            [this](const QString &message) {
        m_syncButton->setEnabled(m_session->hasSession());
        m_cancelButton->setEnabled(false);
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
    m_syncButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
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
        m_table->setItem(row, 0, check);
        auto *name = new QTableWidgetItem(group.displayName);
        name->setToolTip(group.marketHashName);
        m_table->setItem(row, 1, name);
        m_table->setItem(row, 2, new QTableWidgetItem(categoryText(group.category)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(group.inventoryQuantity)));
        auto *quantity = new QSpinBox(m_table);
        quantity->setRange(1, qMax(1, qMin(group.inventoryQuantity, group.assetIds.size())));
        quantity->setValue(quantity->maximum());
        quantity->setEnabled(false);
        connect(quantity, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
            resetHandoff();
            updateSummary();
        });
        m_table->setCellWidget(row, 4, quantity);
        m_table->setItem(row, 5, new QTableWidgetItem(QStringLiteral("可交接")));
        m_table->setRowHeight(row, 36);
    }
    m_table->blockSignals(false);
    resetHandoff();
    applyFilter();
    updateSummary();
}

void InventoryAssistantPage::applyFilter() {
    const QString query = m_filter->text().trimmed();
    int visibleCount = 0;
    for (int row = 0; row < m_groups.size(); ++row) {
        const InventoryGroup &group = m_groups.at(row);
        const bool queryMatches = query.isEmpty()
                                  || group.displayName.contains(query, Qt::CaseInsensitive)
                                  || group.marketHashName.contains(query, Qt::CaseInsensitive);
        const bool categoryMatches = !m_cardsOnly->isChecked()
                                     || group.category == QLatin1String("trading_card");
        const bool visible = queryMatches && categoryMatches;
        m_table->setRowHidden(row, !visible);
        if (visible) ++visibleCount;
    }
    m_selectionStatus->setText(QStringLiteral("显示 %1 / %2 组物品").arg(visibleCount).arg(m_groups.size()));
}

void InventoryAssistantPage::selectVisible(bool duplicatesOnly) {
    m_table->blockSignals(true);
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->isRowHidden(row)) continue;
        const InventoryGroup &group = m_groups.at(row);
        const bool selected = !duplicatesOnly || group.inventoryQuantity > 1;
        m_table->item(row, 0)->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 4))) {
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
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 4))) {
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
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 4))) {
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
    m_summary->setText(
        QStringLiteral("%1 组 / %2 件 · 买家合计 ¥%3 · 预计实收 ¥%4 · 预计手续费 ¥%5")
            .arg(selected.size())
            .arg(itemCount)
            .arg(totalBuyer / 100.0, 0, 'f', 2)
            .arg(totalReceive / 100.0, 0, 'f', 2)
            .arg(totalFee / 100.0, 0, 'f', 2));
    m_handoffButton->setEnabled(!selected.isEmpty() && m_session->hasSession());
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto *quantity = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 4))) {
            quantity->setEnabled(m_table->item(row, 0)->checkState() == Qt::Checked);
        }
    }
}

void InventoryAssistantPage::resetHandoff() {
    m_batches.clear();
    m_nextBatchIndex = 0;
    if (m_handoffButton) {
        m_handoffButton->setText(QStringLiteral("检查并打开 Steam 官方页面"));
    }
}

void InventoryAssistantPage::prepareHandoff() {
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
