#include "ui/widgets/AccountCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

AccountCard::AccountCard(QWidget *parent) : QFrame(parent) {
  setObjectName(QStringLiteral("accountCard"));
  setMinimumHeight(80);
  setCursor(Qt::PointingHandCursor);

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 12, 8, 12);
  layout->setSpacing(10);
  m_stateIcon = new QLabel(QStringLiteral("○"), this);
  m_stateIcon->setAlignment(Qt::AlignCenter);
  m_stateIcon->setFixedSize(36, 36);
  layout->addWidget(m_stateIcon);

  auto *text = new QVBoxLayout();
  text->setSpacing(2);
  m_title = new QLabel(QStringLiteral("请选择使用方式"), this);
  m_title->setTextFormat(Qt::PlainText);
  m_subtitle = new QLabel(QStringLiteral("游客浏览或登录 Steam"), this);
  m_subtitle->setObjectName(QStringLiteral("mutedText"));
  m_subtitle->setTextFormat(Qt::PlainText);
  m_subtitle->setWordWrap(true);
  text->addWidget(m_title);
  text->addWidget(m_subtitle);
  layout->addLayout(text, 1);

  m_menu = new QPushButton(QStringLiteral("⋯"), this);
  m_menu->setFixedSize(36, 36);
  m_menu->setAccessibleName(QStringLiteral("账户操作"));
  layout->addWidget(m_menu);
  connect(m_menu, &QPushButton::clicked, this, &AccountCard::updateMenuAction);
}

void AccountCard::setSnapshot(const IdentitySnapshot &snapshot) {
  m_snapshot = snapshot;
  switch (snapshot.state) {
    case IdentityState::ChoiceRequired:
      m_stateIcon->setText(QStringLiteral("○"));
      m_stateIcon->setObjectName(QStringLiteral("mutedText"));
      m_title->setText(QStringLiteral("请选择使用方式"));
      m_subtitle->setText(QStringLiteral("游客浏览或登录 Steam"));
      break;
    case IdentityState::Guest:
      m_stateIcon->setText(QStringLiteral("◉"));
      m_stateIcon->setObjectName(QStringLiteral("mutedText"));
      m_title->setText(QStringLiteral("游客模式"));
      m_subtitle->setText(QStringLiteral("公开市场与本地功能可用"));
      break;
    case IdentityState::PublicInventory:
      m_stateIcon->setText(QStringLiteral("◎"));
      m_stateIcon->setObjectName(QStringLiteral("statusLive"));
      m_title->setText(QStringLiteral("公开库存"));
      m_subtitle->setText(snapshot.steamId64.isEmpty()
                              ? QStringLiteral("已选择公开身份")
                              : QStringLiteral("SteamID %1").arg(snapshot.steamId64));
      break;
    case IdentityState::Authenticating:
      m_stateIcon->setText(QStringLiteral("◌"));
      m_stateIcon->setObjectName(QStringLiteral("statusWarning"));
      m_title->setText(QStringLiteral("正在登录 Steam"));
      m_subtitle->setText(QStringLiteral("请在官方页面完成登录"));
      break;
    case IdentityState::Authenticated:
      m_stateIcon->setText(QStringLiteral("●"));
      m_stateIcon->setObjectName(QStringLiteral("statusLive"));
      m_title->setText(snapshot.displayName.isEmpty() ? QStringLiteral("Steam 已登录")
                                                      : snapshot.displayName);
      m_subtitle->setText(m_lastSyncSummary.isEmpty() ? QStringLiteral("本人库存与账户操作可用")
                                                      : m_lastSyncSummary);
      break;
    case IdentityState::Expired:
      m_stateIcon->setText(QStringLiteral("!"));
      m_stateIcon->setObjectName(QStringLiteral("statusWarning"));
      m_title->setText(QStringLiteral("会话已失效"));
      m_subtitle->setText(QStringLiteral("公开市场仍可用 · 请重新登录"));
      break;
  }
  m_title->setToolTip(m_title->text());
  m_subtitle->setToolTip(m_subtitle->text());
  m_menu->setEnabled(snapshot.state != IdentityState::Authenticating);
  style()->unpolish(m_stateIcon);
  style()->polish(m_stateIcon);
}

void AccountCard::setLastSyncSummary(const QString &summary) {
  m_lastSyncSummary = summary;
  setSnapshot(m_snapshot);
}

void AccountCard::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && !m_menu->geometry().contains(event->pos())) {
    emit manageRequested();
  }
  QFrame::mouseReleaseEvent(event);
}

void AccountCard::updateMenuAction() {
  switch (m_snapshot.state) {
    case IdentityState::Authenticated:
    case IdentityState::PublicInventory:
      emit logoutRequested();
      break;
    case IdentityState::Authenticating:
      break;
    default:
      emit loginRequested();
      break;
  }
}

