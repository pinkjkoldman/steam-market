#pragma once

#include <QVector>
#include <QWidget>

#include "core/models/InventoryModels.h"

class InventoryService;
class MultiSellHandoffService;
class PricingDraftService;
class SteamSessionService;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QVBoxLayout;

class InventoryAssistantPage : public QWidget {
    Q_OBJECT

public:
    InventoryAssistantPage(SteamSessionService *session, InventoryService *inventory,
                           PricingDraftService *pricing,
                           MultiSellHandoffService *handoff, QWidget *parent = nullptr);

private:
    void buildUi();
    void buildAccountSection(QVBoxLayout *root);
    void buildInventorySection(QVBoxLayout *root);
    void buildPricingSection(QVBoxLayout *root);
    void wireSignals();
    void onSessionChanged(bool authenticated, const QString &steamId,
                          const QString &displayName);
    void startSync();
    void loadCachedInventory();
    void populateGroups(const QVector<InventoryGroup> &groups);
    void applyFilter();
    void selectVisible(bool duplicatesOnly);
    void clearSelection();
    QVector<InventoryGroup> selectedGroups() const;
    void updateSummary();
    void resetHandoff();
    void prepareHandoff();
    void openNextBatch();
    void setOperationStatus(const QString &text, bool isError = false);
    QPair<int, QString> currentContext() const;

    SteamSessionService *m_session = nullptr;
    InventoryService *m_inventory = nullptr;
    PricingDraftService *m_pricing = nullptr;
    MultiSellHandoffService *m_handoff = nullptr;

    QLabel *m_accountBadge = nullptr;
    QLabel *m_accountDetails = nullptr;
    QLabel *m_operationStatus = nullptr;
    QLabel *m_selectionStatus = nullptr;
    QLabel *m_summary = nullptr;
    QLineEdit *m_publicSteamId = nullptr;
    QLineEdit *m_filter = nullptr;
    QComboBox *m_context = nullptr;
    QCheckBox *m_cardsOnly = nullptr;
    QCheckBox *m_autoSync = nullptr;
    QTableWidget *m_table = nullptr;
    QDoubleSpinBox *m_price = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_logoutButton = nullptr;
    QPushButton *m_publicButton = nullptr;
    QPushButton *m_syncButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPushButton *m_openInventoryButton = nullptr;
    QPushButton *m_handoffButton = nullptr;

    QVector<InventoryGroup> m_groups;
    QVector<HandoffBatch> m_batches;
    int m_nextBatchIndex = 0;
    bool m_authenticated = false;
    QString m_lastAutoSyncKey;
};
