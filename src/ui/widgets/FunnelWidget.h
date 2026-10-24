#pragma once

#include <optional>
#include <QList>
#include <QWidget>
#include "services/StatsService.h"

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::Ui::Widgets {

/// Custom painted hiring funnel widget showing stage counts and interview rate (SPEC §7.2, Amendment B7).
/// Sizes are derived strictly from font metrics with zero per-paint allocations.
class FunnelWidget : public QWidget {
    Q_OBJECT

public:
    explicit FunnelWidget(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    ~FunnelWidget() override = default;

    void setFunnel(const QList<Services::FunnelStage>& stages,
                   const std::optional<double>& interviewRate);

    const QList<Services::FunnelStage>& stages() const { return m_stages; }
    std::optional<double> interviewRate() const { return m_interviewRate; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateAccessibility();

    Theme::ThemeManager& m_theme;
    QList<Services::FunnelStage> m_stages;
    std::optional<double> m_interviewRate;
};

}  // namespace JobPrep::Ui::Widgets
