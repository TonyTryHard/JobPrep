#include "ui/widgets/ProgressRing.h"

#include <QFontMetrics>
#include <QHelpEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

ProgressRing::ProgressRing(Theme::ThemeManager& theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this] { update(); });
    updateAccessibility();
}

void ProgressRing::setTracks(const QList<Services::TrackProgressStat>& tracks) {
    m_tracks = tracks;
    updateAccessibility();
    updateGeometry();
    update();
}

void ProgressRing::setTrack(const Services::TrackProgressStat& track) {
    setTracks({track});
}

void ProgressRing::updateAccessibility() {
    setAccessibleName(tr("Study progress rings"));
    QStringList descParts;
    for (const auto& track : m_tracks) {
        descParts.append(u"%1: %2%"_s.arg(track.trackName).arg(track.percent));
    }
    setAccessibleDescription(descParts.join(u", "_s));
}

int ProgressRing::columnsForWidth(int width) const {
    const QFontMetrics fm(font());
    const int ringDiameter = qMax(44, static_cast<int>(fm.height() * 3.6));
    const int cellWidth = ringDiameter + qMax(16, fm.height());
    const int gapX = qMax(12, fm.height());

    const int availableW = qMax(cellWidth, width);
    const int cols = qMax(1, (availableW + gapX) / (cellWidth + gapX));
    return cols;
}

QRect ProgressRing::ringRect(int index, int width) const {
    if (index < 0 || index >= m_tracks.size()) return QRect();

    const QFontMetrics fm(font());
    const int ringDiameter = qMax(44, static_cast<int>(fm.height() * 3.6));
    const int cellWidth = ringDiameter + qMax(16, fm.height());
    const int cellHeight = ringDiameter + fm.height() + qMax(6, fm.height() / 3);
    const int gapX = qMax(12, fm.height());
    const int gapY = qMax(12, fm.height());

    const int cols = columnsForWidth(width);
    const int col = index % cols;
    const int row = index / cols;

    const int x = col * (cellWidth + gapX);
    const int y = row * (cellHeight + gapY);
    return QRect(x, y, cellWidth, cellHeight);
}

QSize ProgressRing::sizeHint() const {
    const QFontMetrics fm(font());
    const int ringDiameter = qMax(44, static_cast<int>(fm.height() * 3.6));
    const int cellWidth = ringDiameter + qMax(16, fm.height());
    const int cellHeight = ringDiameter + fm.height() + qMax(6, fm.height() / 3);
    const int gapX = qMax(12, fm.height());
    const int gapY = qMax(12, fm.height());

    const int count = m_tracks.isEmpty() ? 1 : static_cast<int>(m_tracks.size());
    const int cols = qBound(1, count, 4);
    const int rows = (count + cols - 1) / cols;

    return {cols * cellWidth + (cols - 1) * gapX, rows * cellHeight + (rows - 1) * gapY};
}

QSize ProgressRing::minimumSizeHint() const {
    const QFontMetrics fm(font());
    const int ringDiameter = qMax(44, static_cast<int>(fm.height() * 3.6));
    const int cellWidth = ringDiameter + qMax(16, fm.height());
    const int cellHeight = ringDiameter + fm.height() + qMax(6, fm.height() / 3);
    return {cellWidth, cellHeight};
}

bool ProgressRing::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);
        for (int i = 0; i < m_tracks.size(); ++i) {
            const QRect rect = ringRect(i, width());
            if (rect.contains(helpEvent->pos())) {
                const auto& track = m_tracks.at(i);
                QToolTip::showText(helpEvent->globalPos(),
                                   u"%1: %2%"_s.arg(track.trackName).arg(track.percent), this);
                return true;
            }
        }
        QToolTip::hideText();
        event->accept();
        return true;
    }
    return QWidget::event(event);
}

void ProgressRing::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (m_tracks.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto tokens = m_theme.currentTokens();
    const QFontMetrics fm(font());
    const int ringDiameter = qMax(44, static_cast<int>(fm.height() * 3.6));
    const int ringThickness = qMax(4, ringDiameter / 8);
    const int labelHeight = fm.height();
    const int labelGap = qMax(4, fm.height() / 4);

    QFont centerFont = font();
    centerFont.setBold(true);
    const QFontMetrics centerFm(centerFont);

    for (int i = 0; i < m_tracks.size(); ++i) {
        const auto& track = m_tracks.at(i);
        const QRect cell = ringRect(i, width());

        // Center ring within the cell horizontally
        const int ringX = cell.x() + (cell.width() - ringDiameter) / 2;
        const int ringY = cell.y();
        const QRectF ringRectF(ringX + ringThickness / 2.0, ringY + ringThickness / 2.0,
                              ringDiameter - ringThickness, ringDiameter - ringThickness);

        // Background track circle
        painter.setPen(QPen(tokens.border, ringThickness, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(ringRectF);

        // Progress arc
        if (track.percent > 0) {
            QColor arcColor = tokens.accent;
            if (!track.colorHex.isEmpty()) {
                const QColor parsed = QColor::fromString(track.colorHex);
                if (parsed.isValid()) arcColor = parsed;
            }

            painter.setPen(QPen(arcColor, ringThickness, Qt::SolidLine, Qt::RoundCap));
            const int clampedPercent = qBound(0, track.percent, 100);
            const int startAngle = 90 * 16;  // 12 o'clock
            const int spanAngle = -qRound((clampedPercent / 100.0) * 360.0 * 16);
            painter.drawArc(ringRectF, startAngle, spanAngle);
        }

        // Center percentage text
        painter.setFont(centerFont);
        painter.setPen(track.percent == 0 ? tokens.textMuted : tokens.text);
        const QString pctStr = u"%1%"_s.arg(track.percent);
        const QRect textCircleRect(ringX, ringY, ringDiameter, ringDiameter);
        painter.drawText(textCircleRect, Qt::AlignCenter, pctStr);

        // Track name label below ring (elided with tooltip)
        painter.setFont(font());
        painter.setPen(tokens.textMuted);
        const int labelTop = ringY + ringDiameter + labelGap;
        const QRect labelRect(cell.x(), labelTop, cell.width(), labelHeight);
        const QString elidedName = fm.elidedText(track.trackName, Qt::ElideRight, cell.width());
        painter.drawText(labelRect, Qt::AlignCenter, elidedName);
    }
}

}  // namespace JobPrep::Ui::Widgets
