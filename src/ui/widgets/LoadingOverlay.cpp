#include "ui/widgets/LoadingOverlay.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QProgressBar>
#include <QShowEvent>
#include <QVBoxLayout>

LoadingOverlay::LoadingOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);

    m_container = new QWidget(this);
    m_container->setObjectName(QStringLiteral("loadingCard"));
    m_container->setStyleSheet(
        QStringLiteral("QWidget#loadingCard { background:#101F31; border:1px solid #24364A; "
                       "border-radius:12px; padding:24px 32px; }"));

    m_bar = new QProgressBar(m_container);
    m_bar->setRange(0, 0);
    m_bar->setTextVisible(false);
    m_bar->setFixedWidth(160);

    m_label = new QLabel(QStringLiteral("加载中…"), m_container);
    m_label->setObjectName(QStringLiteral("emptyStateTitle"));
    m_label->setAlignment(Qt::AlignCenter);

    auto *layout = new QVBoxLayout(m_container);
    layout->setSpacing(12);
    layout->addWidget(m_bar, 0, Qt::AlignCenter);
    layout->addWidget(m_label, 0, Qt::AlignCenter);

    hide();
    raise();
}

void LoadingOverlay::setText(const QString &text) {
    m_label->setText(text);
}

void LoadingOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(13, 17, 23, 180));
}

void LoadingOverlay::resizeEvent(QResizeEvent *) {
    center();
}

void LoadingOverlay::center() {
    if (!parentWidget()) return;
    setGeometry(parentWidget()->rect());
    const QSize sz = m_container->sizeHint();
    m_container->move((width() - sz.width()) / 2, (height() - sz.height()) / 2);
}

void LoadingOverlay::showEvent(QShowEvent *) {
    raise();
    center();
}
