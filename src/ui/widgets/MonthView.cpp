#include "ui/widgets/MonthView.h"

#include <QFontMetrics>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include "services/SettingsService.h"
#include "ui/theme/AgendaKindStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

namespace {

constexpr int kColumns = 7;
constexpr int kRows = 6;
constexpr int kCellRadius = 8;
constexpr int kPanelRadius = 12;
constexpr int kDotSize = 6;
constexpr int kDotGap = 4;

QLocale englishLocale() {
    return QLocale(QLocale::English);
}

}  // namespace

MonthView::MonthView(JobPrep::Ui::Theme::ThemeManager& theme,
                     std::function<QDate()> todayProvider, QWidget* parent)
    : QWidget(parent),
      m_theme(theme),
      m_todayProvider(todayProvider ? std::move(todayProvider)
                                    : std::function<QDate()>([] { return QDate::currentDate(); })) {
    m_visibleMonth = QDate(m_todayProvider().year(), m_todayProvider().month(), 1);
    m_selectedDay = m_todayProvider();
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(false);
    setAttribute(Qt::WA_Hover, false);

    connect(&m_theme, &JobPrep::Ui::Theme::ThemeManager::themeChanged, this,
            [this]() { update(); });
}

void MonthView::setFirstDayOfWeek(int day) {
    const int normalized = static_cast<int>(Services::SettingsService::toDayOfWeek(day));
    if (normalized == m_firstDayOfWeek) return;
    m_firstDayOfWeek = normalized;
    update();
}

void MonthView::setTodayProvider(std::function<QDate()> provider) {
    m_todayProvider = provider ? std::move(provider)
                               : std::function<QDate()>([] { return QDate::currentDate(); });
    update();
}

QDate MonthView::today() const {
    return m_todayProvider();
}

void MonthView::setVisibleMonth(QDate month) {
    if (!month.isValid()) return;
    const QDate first(month.year(), month.month(), 1);
    if (first == m_visibleMonth) return;
    m_visibleMonth = first;
    update();
    emit monthChanged(m_visibleMonth);
}

void MonthView::selectDate(QDate day) {
    if (!day.isValid()) {
        const bool dayChanged = m_selectedDay.isValid();
        m_selectedDay = QDate();
        update();
        if (dayChanged) emit selectionChanged(m_selectedDay);
        return;
    }
    const QDate targetMonth(day.year(), day.month(), 1);
    bool monthMoved = false;
    if (targetMonth != m_visibleMonth) {
        m_visibleMonth = targetMonth;
        monthMoved = true;
    }
    const bool dayChanged = (day != m_selectedDay);
    m_selectedDay = day;
    update();
    if (monthMoved) emit monthChanged(m_visibleMonth);
    if (dayChanged || monthMoved) emit selectionChanged(day);
}

void MonthView::setSelectedDay(QDate day) {
    selectDate(day);
}

QPair<QDate, QDate> MonthView::visibleRange() const {
    // Offset from the month's first day back to the start of its first displayed week.
    const int monthStartColumn = (m_visibleMonth.dayOfWeek() - m_firstDayOfWeek + 7) % 7;
    const QDate first = m_visibleMonth.addDays(-monthStartColumn);
    return {first, first.addDays(kColumns * kRows - 1)};
}

void MonthView::setMarkers(const QHash<QDate, DayMarkers>& markers) {
    m_markers = markers;
    update();
}

MonthView::DayMarkers MonthView::markersFor(QDate day) const {
    return m_markers.value(day);
}

QString MonthView::monthTitle() const {
    return englishLocale().toString(m_visibleMonth, u"MMMM yyyy"_s);
}

int MonthView::headerHeight() const {
    return fontMetrics().height() + 12;
}

QRect MonthView::gridRect() const {
    return rect().adjusted(0, headerHeight(), 0, 0);
}

QRect MonthView::cellRect(int row, int column) const {
    const QRect grid = gridRect();
    const int left = grid.left() + (column * grid.width()) / kColumns;
    const int right = grid.left() + ((column + 1) * grid.width()) / kColumns;
    const int top = grid.top() + (row * grid.height()) / kRows;
    const int bottom = grid.top() + ((row + 1) * grid.height()) / kRows;
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

QRect MonthView::dayRect(QDate day) const {
    const auto range = visibleRange();
    const qint64 offset = range.first.daysTo(day);
    if (offset < 0 || offset >= kColumns * kRows) return {};
    return cellRect(static_cast<int>(offset) / kColumns, static_cast<int>(offset) % kColumns);
}

QDate MonthView::dateAt(QPoint position) const {
    const QRect grid = gridRect();
    if (!grid.contains(position) || grid.width() <= 0 || grid.height() <= 0) return {};
    const int column = qBound(0, (position.x() - grid.left()) * kColumns / grid.width(), kColumns - 1);
    const int row = qBound(0, (position.y() - grid.top()) * kRows / grid.height(), kRows - 1);
    return visibleRange().first.addDays(row * kColumns + column);
}

void MonthView::stepSelection(int days) {
    selectDate(m_selectedDay.addDays(days));
}

QDate MonthView::startOfWeek(QDate day) const {
    const int offset = (day.dayOfWeek() - m_firstDayOfWeek + 7) % 7;
    return day.addDays(-offset);
}

void MonthView::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QDate day = dateAt(event->position().toPoint());
    if (!day.isValid()) return;
    setFocus(Qt::MouseFocusReason);
    selectDate(day);
}

void MonthView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    const QDate day = dateAt(event->position().toPoint());
    if (day.isValid()) emit dayActivated(day);
}

void MonthView::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Left:
            stepSelection(-1);
            return;
        case Qt::Key_Right:
            stepSelection(1);
            return;
        case Qt::Key_Up:
            stepSelection(-kColumns);
            return;
        case Qt::Key_Down:
            stepSelection(kColumns);
            return;
        case Qt::Key_PageUp:
            setSelectedDay(m_selectedDay.addMonths(-1));
            return;
        case Qt::Key_PageDown:
            setSelectedDay(m_selectedDay.addMonths(1));
            return;
        case Qt::Key_Home:
            setSelectedDay(startOfWeek(m_selectedDay));
            return;
        case Qt::Key_End:
            setSelectedDay(startOfWeek(m_selectedDay).addDays(kColumns - 1));
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            emit dayActivated(m_selectedDay);
            return;
        default:
            break;
    }
    QWidget::keyPressEvent(event);
}

void MonthView::focusInEvent(QFocusEvent* event) {
    m_hasFocus = true;
    update();
    QWidget::focusInEvent(event);
}

void MonthView::focusOutEvent(QFocusEvent* event) {
    m_hasFocus = false;
    update();
    QWidget::focusOutEvent(event);
}

QSize MonthView::sizeHint() const {
    return {560, 460};
}

QSize MonthView::minimumSizeHint() const {
    return {320, 300};
}

void MonthView::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const auto tokens = m_theme.currentTokens();

    // Panel surface with a subtle border (AGENTS.md §7: border + surface, no drop shadow).
    QRectF panel = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(tokens.border, 1.0));
    painter.setBrush(tokens.surface);
    painter.drawRoundedRect(panel, kPanelRadius, kPanelRadius);

    const QLocale locale = englishLocale();
    const QFont baseFont = font();
    QFont weekdayFont(baseFont);
    weekdayFont.setPointSizeF(qMax(7.5, baseFont.pointSizeF() - 1.0));
    QFont dayFont(baseFont);
    QFont todayFont(baseFont);
    todayFont.setBold(true);

    // Weekday header.
    painter.setFont(weekdayFont);
    painter.setPen(tokens.textMuted);
    for (int column = 0; column < kColumns; ++column) {
        const int qtDay = m_firstDayOfWeek + column > 7 ? m_firstDayOfWeek + column - 7
                                                         : m_firstDayOfWeek + column;
        const QString name = locale.dayName(qtDay, QLocale::ShortFormat);
        painter.drawText(cellRect(0, column).adjusted(0, -headerHeight(), 0, 0),
                         Qt::AlignCenter, name);
    }

    const QDate today = m_todayProvider();
    const auto range = visibleRange();

    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            const QDate date = range.first.addDays(row * kColumns + column);
            const QRect cell = cellRect(row, column);
            const bool otherMonth = date.month() != m_visibleMonth.month();
            const bool isSelected = date == m_selectedDay;
            const bool isToday = date == today;

            if (isSelected) {
                QRectF highlight = QRectF(cell).adjusted(3, 3, -3, -3);
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_hasFocus ? tokens.accent : tokens.surfaceAlt);
                painter.drawRoundedRect(highlight, kCellRadius, kCellRadius);
                if (!m_hasFocus) {
                    painter.setPen(QPen(tokens.border, 1.0));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(highlight, kCellRadius, kCellRadius);
                }
            } else if (isToday) {
                QRectF ring = QRectF(cell).adjusted(3, 3, -3, -3);
                painter.setPen(QPen(tokens.accent, 1.0, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(ring, kCellRadius, kCellRadius);
            }

            QColor textColor = otherMonth ? tokens.textMuted : tokens.text;
            if (isSelected && m_hasFocus) textColor = tokens.accentText;
            painter.setFont(isToday ? todayFont : dayFont);
            painter.setPen(textColor);
            const QRect numberRect = cell.adjusted(0, 4, 0, 0);
            painter.drawText(numberRect, Qt::AlignHCenter | Qt::AlignTop, QString::number(date.day()));

            const DayMarkers markers = m_markers.value(date);
            if (markers.total > 0) {
                const int count = static_cast<int>(qMin<qsizetype>(markers.kinds.size(), 3));
                const int dotsWidth = count * kDotSize + (count - 1) * kDotGap;
                int x = cell.center().x() - dotsWidth / 2;
                const int y = cell.bottom() - kDotSize - 4;
                painter.setPen(Qt::NoPen);
                for (int i = 0; i < count; ++i) {
                    painter.setBrush(Theme::AgendaKindStyle::color(markers.kinds.at(i)));
                    painter.drawEllipse(QRect(x, y, kDotSize, kDotSize));
                    x += kDotSize + kDotGap;
                }
                if (markers.total > count) {
                    painter.setFont(weekdayFont);
                    painter.setPen(tokens.textMuted);
                    painter.drawText(QRect(x + 2, y - 3, 32, kDotSize + 6),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     u"+%1"_s.arg(markers.total - count));
                }
            }
        }
    }
}

}  // namespace JobPrep::Ui::Widgets
