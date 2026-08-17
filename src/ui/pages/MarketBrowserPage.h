#pragma once

#include <QWidget>

#include "ui/widgets/WorkbenchModels.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QTimer;
class QuickInspector;
class ScopeNotice;

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
  void updatePagination();
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
  QLabel *m_filterHint = nullptr;
  QLabel *m_range = nullptr;
  QPushButton *m_previous = nullptr;
  QPushButton *m_next = nullptr;
  QPushButton *m_clear = nullptr;
  QTableView *m_table = nullptr;
  QStandardItemModel *m_model = nullptr;
  QuickInspector *m_inspector = nullptr;
  ScopeNotice *m_notice = nullptr;
};

