#pragma once

#include <QWidget>

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::Ui::Widgets {

/// Horizontal progress bar for weekly study goal (SPEC §7.2, Amendment B7).
/// Bar is capped at 100% visually while real percentage is displayed in text.
class GoalProgressBar : public QWidget {
    Q_OBJECT

public:
    explicit GoalProgressBar(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    ~GoalProgressBar() override = default;

    void setGoal(int weekMinutes, int goalMinutes);

    int weekMinutes() const { return m_weekMinutes; }
    int goalMinutes() const { return m_goalMinutes; }
    int realPercent() const {
        return m_goalMinutes > 0 ? qRound(m_weekMinutes * 100.0 / m_goalMinutes) : 0;
    }
    int cappedPercent() const { return qMin(100, realPercent()); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateAccessibility();

    Theme::ThemeManager& m_theme;
    int m_weekMinutes{0};
    int m_goalMinutes{360};
};

}  // namespace JobPrep::Ui::Widgets
