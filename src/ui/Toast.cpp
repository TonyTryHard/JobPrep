#include "ui/Toast.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>
#include "ui/NotificationText.h"
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

namespace {
constexpr qreal kToastRadius = 12.0;
}  // namespace

Toast::Toast(const Services::Notification& notification,
             Theme::ThemeManager& theme,
             QWidget* parent)
    : QWidget(parent), m_notification(notification), m_theme(theme) {
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);

    const QString title = NotificationText::formatTitle(m_notification);
    const QString message = NotificationText::formatMessage(m_notification);

    setAccessibleName(title + u": "_s + message);
    setAccessibleDescription(message);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 12, 12);
    mainLayout->setSpacing(12);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_iconLabel, 0, Qt::AlignTop);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    m_titleLabel = new QLabel(title, this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    textLayout->addWidget(m_titleLabel);

    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    textLayout->addWidget(m_messageLabel);

    mainLayout->addLayout(textLayout, 1);

    m_closeBtn = new QToolButton(this);
    m_closeBtn->setAutoRaise(true);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    m_closeBtn->setToolTip(tr("Dismiss"));
    m_closeBtn->setAccessibleName(tr("Dismiss"));
    connect(m_closeBtn, &QToolButton::clicked, this, &Toast::dismissed);
    mainLayout->addWidget(m_closeBtn, 0, Qt::AlignTop);

    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, &Toast::applyTheme);
    applyTheme();
}

QSize Toast::sizeHint() const {
    const int w = 320;
    const int h = layout() ? layout()->sizeHint().height() : 64;
    return QSize(w, std::max(64, h));
}

void Toast::applyTheme() {
    const auto tokens = m_theme.currentTokens();

    QPalette pal = palette();
    pal.setColor(QPalette::WindowText, tokens.text);
    m_titleLabel->setPalette(pal);

    QPalette mutedPal = palette();
    mutedPal.setColor(QPalette::WindowText, tokens.textMuted);
    m_messageLabel->setPalette(mutedPal);

    QString iconName;
    QColor iconColor = tokens.accent;
    switch (m_notification.kind) {
        case Services::NotificationKind::Interview:
            iconName = u"agenda-interview"_s;
            break;
        case Services::NotificationKind::FollowUp:
            iconName = u"agenda-followup"_s;
            break;
        case Services::NotificationKind::OverdueFollowUps:
            iconName = u"agenda-target"_s;
            iconColor = tokens.warning;
            break;
    }

    const QIcon notifIcon = Theme::IconProvider::icon(iconName, iconColor);
    m_iconLabel->setPixmap(notifIcon.pixmap(24, 24));

    const QIcon closeIcon = Theme::IconProvider::icon(u"close"_s, tokens.textMuted);
    m_closeBtn->setIcon(closeIcon);

    update();
}

void Toast::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto tokens = m_theme.currentTokens();
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    p.setBrush(tokens.surfaceAlt);
    if (hasFocus()) {
        p.setPen(QPen(tokens.accent, 2.0));
    } else {
        p.setPen(QPen(tokens.border, 1.0));
    }
    p.drawRoundedRect(r, kToastRadius, kToastRadius);
}

void Toast::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit activated(m_notification);
        emit dismissed();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Toast::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit activated(m_notification);
        emit dismissed();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit dismissed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

}  // namespace JobPrep::Ui
