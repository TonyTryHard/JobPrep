#include "ui/InAppNotifier.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QStatusBar>
#include "ui/MainWindow.h"
#include "ui/Toast.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

InAppNotifier::InAppNotifier(MainWindow& window,
                             Theme::ThemeManager& theme,
                             std::function<void(const Services::Notification&)> clickHandler,
                             QObject* parent)
    : QObject(parent),
      m_window(window),
      m_theme(theme),
      m_clickHandler(std::move(clickHandler)) {
    m_window.installEventFilter(this);
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this] {
        applyOverflowTheme();
    });
}

InAppNotifier::~InAppNotifier() {
    m_window.removeEventFilter(this);
}

bool InAppNotifier::notify(const Services::Notification& notification) {
    if (!m_window.isVisible() || m_window.isMinimized()) {
        return false;
    }

    QWidget* const prevFocus = QApplication::focusWidget();

    auto* toast = new Toast(notification, m_theme, &m_window);
    connect(toast, &Toast::activated, this, [this](const Services::Notification& n) {
        if (m_clickHandler) {
            m_clickHandler(n);
        }
    });
    connect(toast, &Toast::dismissed, this, [this, toast]() {
        dismissToast(toast);
    });

    m_toasts.append(toast);
    updateLayout();

    if (prevFocus && QApplication::focusWidget() != prevFocus) {
        prevFocus->setFocus();
    }

    return true;
}

void InAppNotifier::dismissToast(Toast* toast) {
    if (!toast) return;
    m_toasts.removeAll(toast);
    toast->hide();
    toast->deleteLater();
    updateLayout();
}

void InAppNotifier::dismissNewest() {
    if (m_toasts.isEmpty()) return;
    for (auto* t : m_toasts) {
        if (t && t->hasFocus()) {
            dismissToast(t);
            return;
        }
    }
    dismissToast(m_toasts.last());
}

void InAppNotifier::updateLayout() {
    if (m_toasts.isEmpty()) {
        if (m_overflowLabel) {
            m_overflowLabel->hide();
        }
        return;
    }

    const int total = static_cast<int>(m_toasts.size());
    const int visibleCount = std::min(total, 3);
    const int overflowCount = total - visibleCount;

    if (overflowCount > 0) {
        if (!m_overflowLabel) {
            m_overflowLabel = new QLabel(&m_window);
            m_overflowLabel->setAlignment(Qt::AlignCenter);
            QFont f = m_overflowLabel->font();
            f.setBold(true);
            m_overflowLabel->setFont(f);
        }
        m_overflowLabel->setText(tr("+%1 more").arg(overflowCount));
        applyOverflowTheme();
    } else if (m_overflowLabel) {
        m_overflowLabel->hide();
    }

    constexpr int kMargin = 16;
    constexpr int kSpacing = 8;
    constexpr int kMaxWidth = 320;

    const int width = std::min(kMaxWidth, m_window.width() - 2 * kMargin);
    const int x = m_window.width() - width - kMargin;
    int y = m_window.height() - kMargin;

    if (m_window.statusBar() && m_window.statusBar()->isVisible()) {
        y -= m_window.statusBar()->height();
    }

    // Hide any older toasts that exceed the 3 visible
    for (int i = 0; i < total - visibleCount; ++i) {
        m_toasts.at(i)->hide();
    }

    // Stack visible toasts (newest at bottom)
    for (int i = total - 1; i >= total - visibleCount; --i) {
        Toast* t = m_toasts.at(i);
        const int h = t->sizeHint().height();
        y -= h;
        t->setGeometry(x, y, width, h);
        t->show();
        t->raise();
        y -= kSpacing;
    }

    // Position overflow label above the topmost visible toast
    if (overflowCount > 0 && m_overflowLabel) {
        const int oh = m_overflowLabel->sizeHint().height() + 6;
        const int ow = m_overflowLabel->sizeHint().width() + 16;
        y -= oh;
        m_overflowLabel->setGeometry(x + width - ow, y, ow, oh);
        m_overflowLabel->show();
        m_overflowLabel->raise();
    }
}

void InAppNotifier::applyOverflowTheme() {
    if (!m_overflowLabel) return;
    const auto tokens = m_theme.currentTokens();
    QPalette pal = m_overflowLabel->palette();
    pal.setColor(QPalette::WindowText, tokens.accent);
    m_overflowLabel->setPalette(pal);
    m_overflowLabel->setStyleSheet(
        u"QLabel { background-color: %1; border: 1px solid %2; border-radius: 6px; padding: 2px 8px; }"_s
            .arg(tokens.surfaceAlt.name(), tokens.border.name()));
}

bool InAppNotifier::eventFilter(QObject* watched, QEvent* event) {
    if (watched == &m_window) {
        if (event->type() == QEvent::Resize) {
            updateLayout();
        }
    }
    return QObject::eventFilter(watched, event);
}

int InAppNotifier::visibleToastCount() const {
    int count = 0;
    for (const auto* t : m_toasts) {
        if (t && t->isVisible()) {
            ++count;
        }
    }
    return count;
}

int InAppNotifier::totalToastCount() const {
    return static_cast<int>(m_toasts.size());
}

bool InAppNotifier::isOverflowVisible() const {
    return m_overflowLabel && m_overflowLabel->isVisible();
}

QString InAppNotifier::overflowText() const {
    return m_overflowLabel ? m_overflowLabel->text() : QString();
}

}  // namespace JobPrep::Ui
