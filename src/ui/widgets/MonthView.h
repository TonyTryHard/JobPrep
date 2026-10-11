#pragma once

#include <functional>
#include <QDate>
#include <QHash>
#include <QList>
#include <QPair>
#include <QRect>
#include <QWidget>
#include "domain/Enums.h"

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Widgets {

/// Custom-painted month grid (grid only: the month title and prev/next/today controls
/// are real widgets in `CalendarPage`, Amendment C1). Draws six full weeks including the
/// leading and trailing days of the neighbour months; `visibleRange()` covers all 42 cells.
class MonthView : public QWidget {
    Q_OBJECT

public:
    /// Markers for one day: the distinct kinds present (at most three, for the dots) and
    /// the total item count so "+N" can be painted (Amendment C4).
    struct DayMarkers {
        QList<Domain::AgendaKind> kinds;
        int total{0};
    };

    explicit MonthView(JobPrep::Ui::Theme::ThemeManager& theme,
                       std::function<QDate()> todayProvider = {}, QWidget* parent = nullptr);
    ~MonthView() override = default;

    void setFirstDayOfWeek(int day);
    int firstDayOfWeek() const { return m_firstDayOfWeek; }

    void setTodayProvider(std::function<QDate()> provider);
    QDate today() const;

    /// Any day in the displayed month; the stored value is normalized to its first day.
    QDate visibleMonth() const { return m_visibleMonth; }
    void setVisibleMonth(QDate month);

    QDate selectedDay() const { return m_selectedDay; }
    /// Selects `day`, following into the neighbour month when needed (Amendment C2).
    void setSelectedDay(QDate day);

    /// The 42 days currently shown, `first` to `last` inclusive (Amendment C3).
    QPair<QDate, QDate> visibleRange() const;

    void setMarkers(const QHash<QDate, DayMarkers>& markers);
    DayMarkers markersFor(QDate day) const;

    /// Screen rect of `day` when it falls in the visible grid, otherwise a null rect.
    QRect dayRect(QDate day) const;

    /// Localized "October 2026" title for the visible month.
    QString monthTitle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void dayActivated(QDate day);
    void selectionChanged(QDate day);
    void monthChanged(QDate firstOfMonth);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    int headerHeight() const;
    QRect gridRect() const;
    QRect cellRect(int row, int column) const;
    QDate dateAt(QPoint position) const;
    QDate startOfWeek(QDate day) const;
    void selectDate(QDate day);
    void stepSelection(int days);

    JobPrep::Ui::Theme::ThemeManager& m_theme;
    std::function<QDate()> m_todayProvider;
    QDate m_visibleMonth;
    QDate m_selectedDay;
    int m_firstDayOfWeek{1};  // Qt::Monday
    QHash<QDate, DayMarkers> m_markers;
    bool m_hasFocus{false};
};

}  // namespace JobPrep::Ui::Widgets
