#pragma once

#include <QWidget>

#include "ui/widgets/WorkbenchModels.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QuickInspector;
class ScopeNotice;
class LoadingOverlay;

class MarketOverviewPage : public QWidget {
  Q_OBJECT

public:
  explicit MarketOverviewPage(QWidget *parent = nullptr);

public slots:
  void setScopeSnapshots(const QVector<MarketScopeSnapshotView> &snapshots);
  void setPopularPage(const MarketCatalogPageView &page);
  void setStatus(const MarketUiStatus &status);
  void setInspection(const MarketInspectionView &inspection);
  void setPersonalSummary(int watchCount, int alertCount, const QStringList &recentLines);
  void setCurrency(const QString &code);

signals:
  void scopeSummaryRequested();
  void popularPageRequested(const QString &query, int appid, int offset, int limit,
                            const QString &sort, const QString &currency);
  void itemInspectRequested(int appid, const QString &marketHashName);
  void itemActivated(int appid, const QString &marketHashName);
  void browseAllRequested(const QString &query, int appid);
  void followRequested(int appid, const QString &marketHashName);
  void alertRequested(int appid, const QString &marketHashName);
  void retryRequested();
  void openOfficialMarketRequested();
  void manageWatchlistRequested();
  void manageAlertsRequested();

protected:
  void showEvent(QShowEvent *event) override;

private:
  void requestCurrentScope();
  void selectCurrentRow();
  void activateCurrentRow();
  QString scopeLabel() const;

  bool m_requestedOnce = false;
  QString m_currency = QStringLiteral("CNY");
  QLineEdit *m_search = nullptr;
  QComboBox *m_game = nullptr;
  QLabel *m_totalValue = nullptr;
  QLabel *m_csValue = nullptr;
  QLabel *m_dotaValue = nullptr;
  QLabel *m_communityValue = nullptr;
  QTableView *m_table = nullptr;
  QStandardItemModel *m_model = nullptr;
  QLabel *m_distribution = nullptr;
  QLabel *m_personal = nullptr;
  QPushButton *m_browse = nullptr;
  QuickInspector *m_inspector = nullptr;
  ScopeNotice *m_notice = nullptr;
  LoadingOverlay *m_loading = nullptr;
};

