#pragma once

#include <QFrame>

#include "ui/widgets/WorkbenchModels.h"

class QLabel;
class QPushButton;

class ScopeNotice : public QFrame {
  Q_OBJECT

public:
  explicit ScopeNotice(QWidget *parent = nullptr);

public slots:
  void setPageContext(const MarketCatalogPageView &page, const QString &scopeLabel,
                      const QString &currency);
  void setStatus(const MarketUiStatus &status);

signals:
  void retryRequested();
  void openOfficialMarketRequested();

private:
  QLabel *m_scope = nullptr;
  QLabel *m_freshness = nullptr;
  QLabel *m_status = nullptr;
  QPushButton *m_retry = nullptr;
  QPushButton *m_official = nullptr;
};

