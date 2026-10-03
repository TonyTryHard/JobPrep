#include "ui/Sidebar.h"
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QVBoxLayout>
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

namespace {

/// Custom QPushButton drawing an active accent pill and tinted SVG icons.
class NavButton : public QPushButton {
public:
    NavButton(const QString& iconName, const QString& text, Theme::ThemeManager& themeMgr,
              QWidget* parent = nullptr)
        : QPushButton(text, parent), m_iconName(iconName), m_title(text), m_themeMgr(themeMgr) {
        setCheckable(true);
        setFixedHeight(42);
        setCursor(Qt::PointingHandCursor);
    }

    void setCollapsed(bool collapsed) {
        m_collapsed = collapsed;
        setToolTip(collapsed ? m_title : QString());
        update();
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const auto tokens = m_themeMgr.currentTokens();
        const bool checked = isChecked();
        const bool hovered = underMouse();

        // Background
        if (checked) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(tokens.surfaceAlt);
            painter.drawRoundedRect(rect().adjusted(4, 2, -4, -2), 8, 8);

            // Active accent pill on the left edge
            painter.setBrush(tokens.accent);
            const qreal pillW = 3.5;
            const qreal pillH = 22.0;
            const qreal pillY = (height() - pillH) / 2.0;
            painter.drawRoundedRect(QRectF(6, pillY, pillW, pillH), 1.75, 1.75);
        } else if (hovered) {
            painter.setPen(Qt::NoPen);
            QColor hoverBg = tokens.surfaceAlt;
            hoverBg.setAlpha(120);
            painter.setBrush(hoverBg);
            painter.drawRoundedRect(rect().adjusted(4, 2, -4, -2), 8, 8);
        }

        // Icon resolution & color
        const QColor iconColor = checked ? tokens.accent : (hovered ? tokens.text : tokens.textMuted);
        const QIcon icon = Theme::IconProvider::icon(m_iconName, iconColor);

        constexpr int iconSize = 20;
        if (m_collapsed) {
            const int iconX = (width() - iconSize) / 2;
            const int iconY = (height() - iconSize) / 2;
            icon.paint(&painter, iconX, iconY, iconSize, iconSize);
        } else {
            const int iconX = 18;
            const int iconY = (height() - iconSize) / 2;
            icon.paint(&painter, iconX, iconY, iconSize, iconSize);

            // Text
            painter.setPen(checked ? tokens.accent : (hovered ? tokens.text : tokens.textMuted));
            QFont font = painter.font();
            font.setPointSize(10);
            font.setWeight(checked ? QFont::DemiBold : QFont::Medium);
            painter.setFont(font);

            const QRect textRect(48, 0, width() - 56, height());
            painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_title);
        }
    }

private:
    QString m_iconName;
    QString m_title;
    Theme::ThemeManager& m_themeMgr;
    bool m_collapsed{false};
};

}  // namespace

Sidebar::Sidebar(Theme::ThemeManager& themeMgr, QWidget* parent)
    : QWidget(parent), m_themeMgr(themeMgr) {
    setObjectName(u"SidebarWidget"_s);
    setupUi();

    connect(&m_themeMgr, &Theme::ThemeManager::themeChanged, this, [this]() {
        updateButtons();
        updateToggleIcon();
    });
}

void Sidebar::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 12, 8, 12);
    m_mainLayout->setSpacing(4);

    // Top logo area
    m_logoContainer = new QWidget(this);
    auto* logoLayout = new QHBoxLayout(m_logoContainer);
    logoLayout->setContentsMargins(8, 4, 8, 16);
    logoLayout->setSpacing(10);

    m_logoIcon = new QLabel(m_logoContainer);
    m_logoText = new QLabel(u"JobPrep"_s, m_logoContainer);
    m_logoText->setStyleSheet(u"font-size: 13pt; font-weight: 700;"_s);

    logoLayout->addWidget(m_logoIcon, 0, Qt::AlignCenter);
    logoLayout->addWidget(m_logoText, 1, Qt::AlignVCenter | Qt::AlignLeft);
    m_mainLayout->addWidget(m_logoContainer);

    // Navigation buttons group
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);

    const struct {
        QString iconName;
        QString text;
        int pageIndex;
    } items[] = {
        {u"home"_s, tr("Dashboard"), 0},
        {u"study"_s, tr("Study"), 1},
        {u"jobs"_s, tr("Jobs"), 2},
        {u"calendar"_s, tr("Calendar"), 3},
    };

    for (const auto& item : items) {
        auto* btn = new NavButton(item.iconName, item.text, m_themeMgr, this);
        m_buttonGroup->addButton(btn, item.pageIndex);
        m_mainLayout->addWidget(btn);
        m_navItems.append({btn, item.iconName, item.text, item.pageIndex});

        connect(btn, &QPushButton::clicked, this, [this, idx = item.pageIndex]() {
            setCurrentPage(idx);
            emit pageSelected(idx);
        });
    }

    m_mainLayout->addStretch(1);

    // Settings at bottom
    auto* settingsBtn = new NavButton(u"settings"_s, tr("Settings"), m_themeMgr, this);
    m_buttonGroup->addButton(settingsBtn, 4);
    m_mainLayout->addWidget(settingsBtn);
    m_navItems.append({settingsBtn, u"settings"_s, tr("Settings"), 4});

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        setCurrentPage(4);
        emit pageSelected(4);
    });

    // Toggle button for collapse/expand
    m_toggleBtn = new QPushButton(this);
    m_toggleBtn->setObjectName(u"SidebarToggleBtn"_s);
    m_toggleBtn->setFixedHeight(36);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_toggleBtn, &QPushButton::clicked, this, [this]() {
        setCollapsed(!m_collapsed);
        emit collapseToggled(m_collapsed);
    });
    m_mainLayout->addWidget(m_toggleBtn);

    setCollapsed(false);
    setCurrentPage(0);
    updateButtons();
    updateToggleIcon();
}

void Sidebar::setCollapsed(bool collapsed) {
    m_collapsed = collapsed;
    setFixedWidth(collapsed ? kWidthCollapsed : kWidthExpanded);

    m_logoText->setVisible(!collapsed);

    for (auto& item : m_navItems) {
        auto* navBtn = static_cast<NavButton*>(item.button);
        navBtn->setCollapsed(collapsed);
    }

    updateToggleIcon();
}

void Sidebar::setCurrentPage(int index) {
    m_currentPage = index;
    if (auto* btn = m_buttonGroup->button(index)) {
        btn->setChecked(true);
    }
    update();
}

void Sidebar::updateButtons() {
    const auto tokens = m_themeMgr.currentTokens();
    const QIcon appIcon = Theme::IconProvider::icon(u"app"_s, tokens.accent);
    m_logoIcon->setPixmap(appIcon.pixmap(24, 24));
    update();
}

void Sidebar::updateToggleIcon() {
    const auto tokens = m_themeMgr.currentTokens();
    const QString iconName = m_collapsed ? u"chevron-right"_s : u"chevron-left"_s;
    const QIcon icon = Theme::IconProvider::icon(iconName, tokens.textMuted);
    m_toggleBtn->setIcon(icon);
    m_toggleBtn->setToolTip(m_collapsed ? tr("Expand sidebar") : tr("Collapse sidebar"));
}

}  // namespace JobPrep::Ui
