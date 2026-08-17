#pragma once

#include <QFrame>

#include "core/models/IdentitySnapshot.h"

class QLabel;
class QPushButton;

class AccountCard : public QFrame {
  Q_OBJECT

public:
  explicit AccountCard(QWidget *parent = nullptr);

public slots:
  void setSnapshot(const IdentitySnapshot &snapshot);
  void setLastSyncSummary(const QString &summary);

signals:
  void loginRequested();
  void logoutRequested();
  void manageRequested();

protected:
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  void updateMenuAction();

  IdentitySnapshot m_snapshot;
  QString m_lastSyncSummary;
  QLabel *m_stateIcon = nullptr;
  QLabel *m_title = nullptr;
  QLabel *m_subtitle = nullptr;
  QPushButton *m_menu = nullptr;
};

