#pragma once

#include <QList>
#include <QWidget>
#include "services/StatsService.h"

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::Ui::Widgets {

/// Custom painted wrapping progress rings for tracks and overall progress (SPEC §7.2, Amendment B7).
/// Sizes are derived strictly from font metrics with zero per-paint allocations.
class ProgressRing : public QWidget {
    Q_OBJECT

public:
    explicit ProgressRing(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    ~ProgressRing() override = default;

    void setTracks(const QList<Services::TrackProgressStat>& tracks);
    void setTrack(const Services::TrackProgressStat& track);
    const QList<Services::TrackProgressStat>& tracks() const { return m_tracks; }

    /// Returns the number of columns computed for the given width.
    int columnsForWidth(int width) const;

    /// Returns the bounding rect of ring at index, for tests and tooltips.
    QRect ringRect(int index, int width) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void updateAccessibility();

    Theme::ThemeManager& m_theme;
    QList<Services::TrackProgressStat> m_tracks;
};

}  // namespace JobPrep::Ui::Widgets
