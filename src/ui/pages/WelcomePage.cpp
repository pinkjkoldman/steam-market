#include "ui/pages/WelcomePage.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "ui/widgets/ModeCard.h"
#include "ui/widgets/WorkbenchTheme.h"

WelcomePage::WelcomePage(QWidget *parent) : QWidget(parent) {
  setProperty("workbench", true);
  setStyleSheet(WorkbenchTheme::styleSheet());
  setMinimumSize(760, 560);

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(48, 32, 48, 28);
  root->setSpacing(20);

  auto *brand = new QHBoxLayout();
  auto *mark = new QLabel(QStringLiteral("◉"), this);
  mark->setObjectName(QStringLiteral("dataValue"));
  mark->setFixedSize(48, 48);
  mark->setAlignment(Qt::AlignCenter);
  auto *product = new QLabel(QStringLiteral("Steam 行情终端"), this);
  product->setObjectName(QStringLiteral("sectionTitle"));
  auto *safety = new QLabel(QStringLiteral("公开市场 · 本地优先"), this);
  safety->setObjectName(QStringLiteral("badge"));
  brand->addWidget(mark);
  brand->addWidget(product);
  brand->addStretch();
  brand->addWidget(safety);
  root->addLayout(brand);
  root->addSpacing(28);

  auto *title = new QLabel(QStringLiteral("选择你的使用方式"), this);
  title->setObjectName(QStringLiteral("pageTitle"));
  title->setAlignment(Qt::AlignCenter);
  auto *subtitle = new QLabel(
      QStringLiteral("两种方式都可以浏览 Steam 公开市场；仅本人库存和账户操作需要登录。"),
      this);
  subtitle->setObjectName(QStringLiteral("pageSubtitle"));
  subtitle->setAlignment(Qt::AlignCenter);
  subtitle->setWordWrap(true);
  root->addWidget(title);
  root->addWidget(subtitle);

  auto *cards = new QHBoxLayout();
  cards->setSpacing(24);
  m_guestCard = new ModeCard(ModeCard::Variant::Guest, this);
  m_guestCard->setRecommended(true);
  m_loginCard = new ModeCard(ModeCard::Variant::Steam, this);
  cards->addWidget(m_guestCard, 1);
  cards->addWidget(m_loginCard, 1);
  root->addLayout(cards, 1);

  auto *footer = new QHBoxLayout();
  m_rememberGuest = new QCheckBox(QStringLiteral("下次直接以游客模式进入"), this);
  auto *privacy = new QLabel(
      QStringLiteral("登录仅在 Steam 官方页面完成，应用不接收或保存密码。可随时切换。"), this);
  privacy->setObjectName(QStringLiteral("mutedText"));
  privacy->setWordWrap(true);
  footer->addWidget(m_rememberGuest);
  footer->addStretch();
  footer->addWidget(privacy);
  root->addLayout(footer);

  connect(m_guestCard, &ModeCard::actionRequested, this, &WelcomePage::guestRequested);
  connect(m_loginCard, &ModeCard::actionRequested, this, &WelcomePage::loginRequested);
  connect(m_rememberGuest, &QCheckBox::toggled, this, &WelcomePage::rememberGuestChanged);
}

void WelcomePage::setLoginBusy(bool busy) { m_loginCard->setBusy(busy); }

void WelcomePage::setLoginAvailable(bool available) {
  m_loginCard->setActionEnabled(available);
  m_loginCard->setErrorText(available ? QString() : QStringLiteral(
      "当前无法打开 Steam 官方登录页面。你仍可使用游客模式。"));
}

void WelcomePage::setLoginError(const QString &errorText) {
  m_loginCard->setBusy(false);
  m_loginCard->setErrorText(errorText);
}

