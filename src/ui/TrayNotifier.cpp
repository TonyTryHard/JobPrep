#include "ui/TrayNotifier.h"

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMenu>
#include "ui/InAppNotifier.h"
#include "ui/MainWindow.h"
#include "ui/NotificationText.h"
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcReminders)

namespace JobPrep::Ui {

TrayNotifier::TrayNotifier(MainWindow& window,
                           std::function<void(const Services::Notification&)> clickHandler,
                           QObject* parent)
    : QObject(parent),
      m_window(window),
      m_clickHandler(std::move(clickHandler)),
      m_trayIcon(this) {
    setupTray();
}

TrayNotifier::~TrayNotifier() {
    m_window.setTrayMessageCallback({});
}

void TrayNotifier::setupTray() {
    m_trayMenu = new QMenu(&m_window);
    auto* showAction = m_trayMenu->addAction(tr("Show JobPrep"));
    connect(showAction, &QAction::triggered, this, [this]() {
        m_window.bringToFront();
    });

    auto* quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, &m_window, &MainWindow::quitApplication);

    m_trayIcon.setContextMenu(m_trayMenu);

    m_window.setTrayMessageCallback([this](const QString& title, const QString& message) {
        m_trayIcon.showMessage(title, message, QSystemTrayIcon::Information, 5000);
    });

    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            m_window.bringToFront();
        }
    });

    connect(&m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayNotifier::triggerMessageClick);

    connect(&m_window.themeManager(), &Theme::ThemeManager::themeChanged, this, [this]() {
        updateIcon();
    });

    updateIcon();
    m_trayIcon.show();
}

void TrayNotifier::updateIcon() {
    const auto tokens = m_window.themeManager().currentTokens();
    m_trayIcon.setIcon(Theme::IconProvider::icon(u"app"_s, tokens.accent));
}

bool TrayNotifier::notify(const Services::Notification& notification) {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qCDebug(lcReminders) << "notify: tray unavailable, delivery failed";
        return false;
    }
    if (!m_trayIcon.isVisible()) {
        qCDebug(lcReminders) << "notify: tray icon not visible, delivery failed";
        return false;
    }
    if (!QSystemTrayIcon::supportsMessages()) {
        if (!m_inAppFallback) {
            m_inAppFallback = std::make_unique<InAppNotifier>(m_window, m_window.themeManager(), m_clickHandler);
        }
        qCDebug(lcReminders) << "notify: tray messages unsupported, falling back to in-app toast";
        return m_inAppFallback->notify(notification);
    }

    m_lastNotification = notification;
    const QString title = NotificationText::formatTitle(notification);
    const QString message = NotificationText::formatMessage(notification);

    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
    switch (notification.kind) {
        case Services::NotificationKind::Interview:
        case Services::NotificationKind::FollowUp:
        case Services::NotificationKind::OverdueFollowUps:
            icon = QSystemTrayIcon::Warning;
            break;
    }

    m_trayIcon.showMessage(title, message, icon, 10000);
    return true;
}

void TrayNotifier::triggerMessageClick() {
    if (m_lastNotification) {
        emit notificationActivated(*m_lastNotification);
        if (m_clickHandler) {
            m_clickHandler(*m_lastNotification);
        }
    }
}

}  // namespace JobPrep::Ui
