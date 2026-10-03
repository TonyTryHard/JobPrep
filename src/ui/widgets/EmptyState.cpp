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
    m_layout->setContentsMargins(32, 48, 32, 48);
    m_layout->setSpacing(16);
    m_layout->setAlignment(Qt::AlignCenter);

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

    m_actionBtn = new QPushButton(this);
    m_actionBtn->setProperty("primary", true);
    m_actionBtn->setVisible(false);
    connect(m_actionBtn, &QPushButton::clicked, this, &EmptyState::actionClicked);
    m_layout->addWidget(m_actionBtn, 0, Qt::AlignCenter);
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
    m_iconLabel->setPixmap(icon.pixmap(48, 48));
    m_iconLabel->setVisible(true);
}

}  // namespace JobPrep::Ui::Widgets
