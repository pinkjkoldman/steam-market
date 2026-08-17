#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class ModeCard;

class WelcomePage : public QWidget {
  Q_OBJECT

public:
  explicit WelcomePage(QWidget *parent = nullptr);

public slots:
  void setLoginBusy(bool busy);
  void setLoginAvailable(bool available);
  void setLoginError(const QString &errorText);

signals:
  void guestRequested();
  void loginRequested();
  void rememberGuestChanged(bool remember);

private:
  ModeCard *m_guestCard = nullptr;
  ModeCard *m_loginCard = nullptr;
  QCheckBox *m_rememberGuest = nullptr;
};

