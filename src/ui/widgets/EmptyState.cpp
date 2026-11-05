#include "ui/widgets/EmptyState.h"
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>
#include "ui/theme/IconProvider.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

EmptyState::EmptyState(const QString& iconName, const QString& title, const QString& description,
                       const QString& actionText, QWidget* parent)
    : QWidget(parent), m_iconName(iconName) {
    setObjectName(u"EmptyStateWidget"_s);
    setupUi();
    setTitle(title);
    setDescription(description);
    setActionText(actionText);
    updateIcon();
}

void EmptyState::setupUi() {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(8);
    m_layout->setAlignment(Qt::AlignVCenter);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName(u"EmptyStateIcon"_s);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_iconLabel);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(u"EmptyStateTitle"_s);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);
    m_layout->addWidget(m_titleLabel);

    m_descLabel = new QLabel(this);
    m_descLabel->setObjectName(u"EmptyStateDescription"_s);
    m_descLabel->setAlignment(Qt::AlignCenter);
    m_descLabel->setWordWrap(true);
    m_layout->addWidget(m_descLabel);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);
    btnLayout->setAlignment(Qt::AlignCenter);

    m_actionBtn = new QPushButton(this);
    m_actionBtn->setObjectName(u"EmptyStateActionBtn"_s);
    m_actionBtn->setProperty("primary", true);
    m_actionBtn->setVisible(false);
    connect(m_actionBtn, &QPushButton::clicked, this, &EmptyState::actionClicked);
    btnLayout->addWidget(m_actionBtn);

    m_secondaryActionBtn = new QPushButton(this);
    m_secondaryActionBtn->setObjectName(u"EmptyStateSecondaryActionBtn"_s);
    m_secondaryActionBtn->setVisible(false);
    connect(m_secondaryActionBtn, &QPushButton::clicked, this, &EmptyState::secondaryActionClicked);
    btnLayout->addWidget(m_secondaryActionBtn);

    m_layout->addLayout(btnLayout);
}

void EmptyState::setIcon(const QString& iconName) {
    m_iconName = iconName;
    updateIcon();
}

void EmptyState::setTitle(const QString& title) {
    m_titleLabel->setText(title);
}

void EmptyState::setDescription(const QString& description) {
    m_descLabel->setText(description);
    m_descLabel->setVisible(!description.isEmpty());
}

void EmptyState::setActionText(const QString& actionText) {
    m_actionBtn->setText(actionText);
    m_actionBtn->setVisible(!actionText.isEmpty());
}

void EmptyState::setSecondaryActionText(const QString& actionText) {
    m_secondaryActionBtn->setText(actionText);
    m_secondaryActionBtn->setVisible(!actionText.isEmpty());
}

QSize EmptyState::sizeHint() const {
    return minimumSizeHint();
}

QSize EmptyState::minimumSizeHint() const {
    int w = 0;
    int h = 0;
    const int spacing = m_layout ? m_layout->spacing() : 8;
    const QMargins margins = m_layout ? m_layout->contentsMargins() : QMargins(16, 16, 16, 16);

    if (!m_iconName.isEmpty()) {
        const QSize iconSz = (m_iconLabel && m_iconLabel->minimumSize().isValid() && !m_iconLabel->minimumSize().isEmpty())
                                 ? m_iconLabel->minimumSize()
                                 : QSize(48, 48);
        w = std::max(w, iconSz.width());
        h += (h > 0 ? spacing : 0) + iconSz.height();
    }

    if (m_titleLabel && !m_titleLabel->text().isEmpty()) {
        const int titleW = m_titleLabel->sizeHint().width();
        w = std::max(w, titleW);
        const int titleH = m_titleLabel->sizeHint().height();
        h += (h > 0 ? spacing : 0) + titleH;
    }

    if (m_descLabel && !m_descLabel->text().isEmpty()) {
        const int descW = m_descLabel->sizeHint().width();
        w = std::max(w, descW);
        const int descH = m_descLabel->sizeHint().height();
        h += (h > 0 ? spacing : 0) + descH;
    }

    const bool hasAction = m_actionBtn && !m_actionBtn->text().isEmpty();
    const bool hasSec = m_secondaryActionBtn && !m_secondaryActionBtn->text().isEmpty();
    if (hasAction || hasSec) {
        int btnW = 0;
        int btnH = 0;
        if (hasAction) {
            btnW += m_actionBtn->sizeHint().width();
            btnH = std::max(btnH, m_actionBtn->sizeHint().height());
        }
        if (hasSec) {
            btnW += (btnW > 0 ? 8 : 0) + m_secondaryActionBtn->sizeHint().width();
            btnH = std::max(btnH, m_secondaryActionBtn->sizeHint().height());
        }
        w = std::max(w, btnW);
        h += (h > 0 ? spacing : 0) + btnH;
    }

    w += margins.left() + margins.right();
    h += margins.top() + margins.bottom();
    return QSize(w, h);
}

void EmptyState::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        updateIcon();
    }
    QWidget::changeEvent(event);
}

void EmptyState::showEvent(QShowEvent* event) {
    updateIcon();
    QWidget::showEvent(event);
}

void EmptyState::updateIcon() {
    if (m_iconName.isEmpty()) {
        m_iconLabel->clear();
        m_iconLabel->setVisible(false);
        return;
    }
    const QColor iconColor = palette().color(QPalette::PlaceholderText);
    const QIcon icon = Theme::IconProvider::icon(m_iconName, iconColor);
    const QPixmap px = icon.pixmap(48, 48);
    m_iconLabel->setPixmap(px);
    // Lock minimum to 48x48 logical pixels so layouts cannot shrink the icon below it.
    m_iconLabel->setMinimumSize(QSize(48, 48));
    m_iconLabel->setVisible(true);
}

}  // namespace JobPrep::Ui::Widgets
