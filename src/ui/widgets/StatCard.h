#pragma once

#include <QWidget>

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::Ui::Widgets {

/// Custom painted KPI card for dashboard metrics (SPEC §7.2, Amendment B7).
/// Sizes are derived strictly from font metrics with zero per-paint allocations.
class StatCard : public QWidget {
    Q_OBJECT

public:
    explicit StatCard(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    ~StatCard() override = default;

    void setTitle(const QString& title);
    void setValue(const QString& value);
    void setSubtitle(const QString& subtitle = QString());

    QString title() const { return m_title; }
    QString value() const { return m_value; }
    QString subtitle() const { return m_subtitle; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateAccessibility();

    Theme::ThemeManager& m_theme;
    QString m_title;
    QString m_value;
    QString m_subtitle;
};

}  // namespace JobPrep::Ui::Widgets
