#pragma once

#include <functional>
#include <QList>
#include <QObject>
#include "services/INotifier.h"

class QLabel;

namespace JobPrep::Ui {
class MainWindow;
class Toast;
}  // namespace JobPrep::Ui

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui {

/// Concrete in-app toast notification manager (SPEC §8, Amendment D2).
class InAppNotifier : public QObject, public Services::INotifier {
    Q_OBJECT

public:
    explicit InAppNotifier(MainWindow& window,
                           Theme::ThemeManager& theme,
                           std::function<void(const Services::Notification&)> clickHandler = {},
                           QObject* parent = nullptr);
    ~InAppNotifier() override;

    bool notify(const Services::Notification& notification) override;

    void dismissToast(Toast* toast);
    void dismissNewest();
    void updateLayout();

    int visibleToastCount() const;
    int totalToastCount() const;
    bool isOverflowVisible() const;
    QString overflowText() const;
    const QList<Toast*>& activeToasts() const { return m_toasts; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void applyOverflowTheme();

    MainWindow& m_window;
    Theme::ThemeManager& m_theme;
    std::function<void(const Services::Notification&)> m_clickHandler;

    QList<Toast*> m_toasts;
    QLabel* m_overflowLabel{nullptr};
};

}  // namespace JobPrep::Ui
