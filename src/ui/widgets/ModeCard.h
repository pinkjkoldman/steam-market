#pragma once

#include <QFrame>
#include <QStringList>

class QLabel;
class QPushButton;

class ModeCard : public QFrame {
  Q_OBJECT

public:
  enum class Variant { Guest, Steam };

  explicit ModeCard(Variant variant, QWidget *parent = nullptr);
  void setRecommended(bool recommended);
  void setBusy(bool busy);
  void setActionEnabled(bool enabled);
  void setErrorText(const QString &errorText);

signals:
  void actionRequested();

private:
  QString m_actionText;
  QLabel *m_badge = nullptr;
  QLabel *m_error = nullptr;
  QPushButton *m_action = nullptr;
};

