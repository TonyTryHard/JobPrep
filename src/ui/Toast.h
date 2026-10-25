#pragma once

#include <QWidget>
#include "services/INotifier.h"

class QLabel;
class QToolButton;

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui {

/// In-app sticky toast notification widget (SPEC §8, Amendment D2).
class Toast : public QWidget {
    Q_OBJECT

public:
    explicit Toast(const Services::Notification& notification,
                   Theme::ThemeManager& theme,
                   QWidget* parent = nullptr);
    ~Toast() override = default;

    const Services::Notification& notification() const { return m_notification; }

    QSize sizeHint() const override;

signals:
    void activated(const Services::Notification& notification);
    void dismissed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void applyTheme();

    Services::Notification m_notification;
    Theme::ThemeManager& m_theme;

    QLabel* m_iconLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_messageLabel{nullptr};
    QToolButton* m_closeBtn{nullptr};
};

}  // namespace JobPrep::Ui
