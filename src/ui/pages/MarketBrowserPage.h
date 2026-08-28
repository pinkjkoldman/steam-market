#pragma once

#include <QWidget>

#include "ui/widgets/WorkbenchModels.h"

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStandardItemModel;
class QTableView;
class QTimer;
class QuickInspector;
class ScopeNotice;
class LoadingOverlay;

class MarketBrowserPage : public QWidget {
  Q_OBJECT

public:
  explicit MarketBrowserPage(QWidget *parent = nullptr);
  void setInitialQuery(const QString &query, int appid);

public slots:
  void setPage(const MarketCatalogPageView &page);
  void setStatus(const MarketUiStatus &status);
  void setInspection(const MarketInspectionView &inspection);
  void focusSearch();
  void setCurrency(const QString &code);

signals:
  void catalogRequested(const QString &query, int appid, int offset, int limit,
                        const QString &sort, const QString &currency);
  void itemInspectRequested(int appid, const QString &marketHashName);
  void itemActivated(int appid, const QString &marketHashName);
  void followRequested(int appid, const QString &marketHashName);
  void alertRequested(int appid, const QString &marketHashName);
  void retryRequested();
  void openOfficialMarketRequested();

protected:
  void showEvent(QShowEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void requestPage(int offset);
  void selectCurrentRow();
  void activateCurrentRow();
  void clearFilters();
  void applyLocalFilters();
  void applyPreset(int index);
  void updatePagination();
  void updateFilterSummary(int visibleCount);
  QString scopeLabel() const;

  bool m_requestedOnce = false;
  int m_offset = 0;
  int m_pageSize = 10;
  qint64 m_totalCount = 0;
  QString m_currency = QStringLiteral("CNY");
  QTimer *m_debounce = nullptr;
  QLineEdit *m_search = nullptr;
  QComboBox *m_game = nullptr;
  QComboBox *m_sort = nullptr;
  QComboBox *m_preset = nullptr;
  QLineEdit *m_typeFilter = nullptr;
  QDoubleSpinBox *m_minPrice = nullptr;
  QDoubleSpinBox *m_maxPrice = nullptr;
  QSpinBox *m_minListings = nullptr;
  QCheckBox *m_pricedOnly = nullptr;
  QLabel *m_filterHint = nullptr;
  QLabel *m_totalMetric = nullptr;
  QLabel *m_visibleMetric = nullptr;
  QLabel *m_priceMetric = nullptr;
  QLabel *m_listingsMetric = nullptr;
  QLabel *m_range = nullptr;
  QPushButton *m_previous = nullptr;
  QPushButton *m_next = nullptr;
  QPushButton *m_clear = nullptr;
  QTableView *m_table = nullptr;
  QStandardItemModel *m_model = nullptr;
  QuickInspector *m_inspector = nullptr;
  ScopeNotice *m_notice = nullptr;
  LoadingOverlay *m_loading = nullptr;
  QVector<MarketCatalogItemView> m_sourceItems;
  int m_filteredCount = 0;
};
