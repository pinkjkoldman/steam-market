#include "ui/widgets/ModeCard.h"

#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

ModeCard::ModeCard(Variant variant, QWidget *parent) : QFrame(parent) {
  setObjectName(QStringLiteral("modeCard"));
  setMinimumHeight(286);
  setAccessibleName(variant == Variant::Guest ? QStringLiteral("游客模式")
                                               : QStringLiteral("登录 Steam"));

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(28, 28, 28, 28);
  layout->setSpacing(12);

  m_badge = new QLabel(this);
  m_badge->setObjectName(QStringLiteral("badge"));
  m_badge->setVisible(false);
  layout->addWidget(m_badge, 0, Qt::AlignLeft);

  auto *icon = new QLabel(variant == Variant::Guest ? QStringLiteral("◉")
                                                     : QStringLiteral("↗"), this);
  icon->setAlignment(Qt::AlignCenter);
  icon->setFixedSize(64, 64);
  icon->setObjectName(QStringLiteral("dataValue"));
  layout->addWidget(icon, 0, Qt::AlignLeft);

  auto *title = new QLabel(variant == Variant::Guest ? QStringLiteral("游客模式")
                                                      : QStringLiteral("登录 Steam"), this);
  title->setObjectName(QStringLiteral("pageTitle"));
  layout->addWidget(title);

  auto *description = new QLabel(
      variant == Variant::Guest
          ? QStringLiteral("无需登录即可浏览 Steam 公开市场、使用本地关注与价格分析。")
          : QStringLiteral("通过 Steam 官方页面登录，以访问本人库存和账户相关操作。"),
      this);
  description->setObjectName(QStringLiteral("mutedText"));
  description->setWordWrap(true);
  layout->addWidget(description);

  const QStringList capabilities =
      variant == Variant::Guest
          ? QStringList{QStringLiteral("✓ Steam 全市场浏览"), QStringLiteral("✓ 本地关注与提醒"),
                        QStringLiteral("✓ 可输入 SteamID 查看公开库存")}
          : QStringList{QStringLiteral("✓ 包含游客模式的全部能力"), QStringLiteral("✓ 查看本人库存"),
                        QStringLiteral("✓ 打开官方账户操作")};
  for (const QString &capability : capabilities) {
    auto *label = new QLabel(capability, this);
    label->setObjectName(QStringLiteral("mutedText"));
    layout->addWidget(label);
  }
  layout->addStretch();

  m_error = new QLabel(this);
  m_error->setObjectName(QStringLiteral("statusError"));
  m_error->setWordWrap(true);
  m_error->setVisible(false);
  layout->addWidget(m_error);

  m_actionText = variant == Variant::Guest ? QStringLiteral("以游客身份进入")
                                            : QStringLiteral("登录 Steam");
  m_action = new QPushButton(m_actionText, this);
  m_action->setObjectName(QStringLiteral("primaryButton"));
  m_action->setMinimumHeight(44);
  m_action->setMinimumWidth(152);
  layout->addWidget(m_action, 0, Qt::AlignLeft);
  connect(m_action, &QPushButton::clicked, this, &ModeCard::actionRequested);
}

void ModeCard::setRecommended(bool recommended) {
  setProperty("recommended", recommended);
  m_badge->setText(QStringLiteral("推荐"));
  m_badge->setVisible(recommended);
  style()->unpolish(this);
  style()->polish(this);
}

void ModeCard::setBusy(bool busy) {
  m_action->setEnabled(!busy);
  m_action->setText(busy ? QStringLiteral("正在打开官方页面…") : m_actionText);
}

void ModeCard::setActionEnabled(bool enabled) { m_action->setEnabled(enabled); }

void ModeCard::setErrorText(const QString &errorText) {
  m_error->setText(errorText);
  m_error->setVisible(!errorText.isEmpty());
}

