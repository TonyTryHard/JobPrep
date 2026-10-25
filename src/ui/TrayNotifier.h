#pragma once

#include <functional>
#include <optional>
#include <QObject>
#include <QSystemTrayIcon>
#include "services/INotifier.h"

class QMenu;

namespace JobPrep::Ui {
class MainWindow;

/// Concrete system tray notification provider (SPEC §8, Amendment D1).
class TrayNotifier : public QObject, public Services::INotifier {
    Q_OBJECT

public:
    explicit TrayNotifier(MainWindow& window,
                          std::function<void(const Services::Notification&)> clickHandler = {},
                          QObject* parent = nullptr);
    ~TrayNotifier() override = default;

    bool notify(const Services::Notification& notification) override;

    std::optional<Services::Notification> lastNotification() const { return m_lastNotification; }

    /// Testing seam to trigger notification click handler offscreen.
    void triggerMessageClick();

signals:
    void notificationActivated(const Services::Notification& notification);

private:
    void setupTray();
    void updateIcon();

    MainWindow& m_window;
    std::function<void(const Services::Notification&)> m_clickHandler;
    QSystemTrayIcon m_trayIcon;
    QMenu* m_trayMenu{nullptr};
    std::optional<Services::Notification> m_lastNotification;
};

}  // namespace JobPrep::Ui
