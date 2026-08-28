#pragma once

#include <QFrame>

#include "ui/widgets/WorkbenchModels.h"

class QLabel;
class QPushButton;
class QResizeEvent;

class QuickInspector : public QFrame {
  Q_OBJECT

public:
  explicit QuickInspector(QWidget *parent = nullptr);

public slots:
  void setItem(const MarketCatalogItemView &item, const QString &currency);
  void clearItem();
  void setInspection(const MarketInspectionView &inspection);

signals:
  void analysisRequested(int appid, const QString &marketHashName);
  void followRequested(int appid, const QString &marketHashName);
  void alertRequested(int appid, const QString &marketHashName);

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void updateNameText();

  int m_appid = 0;
  QString m_hashName;
  QString m_fullName;
  QLabel *m_name = nullptr;
  QLabel *m_game = nullptr;
  QLabel *m_price = nullptr;
  QLabel *m_listings = nullptr;
  QLabel *m_depth = nullptr;
  QLabel *m_freshness = nullptr;
  QPushButton *m_analysis = nullptr;
  QPushButton *m_follow = nullptr;
  QPushButton *m_alert = nullptr;
};
