#include "ui/widgets/StatCard.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

namespace {
constexpr qreal kCardRadius = 12.0;
}  // namespace

StatCard::StatCard(Theme::ThemeManager& theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this] { update(); });
    updateAccessibility();
}

void StatCard::setTitle(const QString& title) {
    if (m_title == title) return;
    m_title = title;
    updateAccessibility();
    updateGeometry();
    update();
}

void StatCard::setValue(const QString& value) {
    if (m_value == value) return;
    m_value = value;
    updateAccessibility();
    updateGeometry();
    update();
}

void StatCard::setSubtitle(const QString& subtitle) {
    if (m_subtitle == subtitle) return;
    m_subtitle = subtitle;
    updateAccessibility();
    updateGeometry();
    update();
}

void StatCard::updateAccessibility() {
    setAccessibleName(m_title.isEmpty() ? m_value : u"%1: %2"_s.arg(m_title, m_value));
    setAccessibleDescription(m_subtitle);
}

QSize StatCard::sizeHint() const {
    const QFontMetrics titleFm(font());
    QFont valueFont = font();
    valueFont.setPointSizeF(font().pointSizeF() * 1.8);
    valueFont.setBold(true);
    const QFontMetrics valueFm(valueFont);

    const int padX = qMax(16, titleFm.height());
    const int padY = qMax(14, static_cast<int>(titleFm.height() * 0.9));
    const int gap = qMax(4, titleFm.height() / 4);

    int textWidth = qMax(titleFm.horizontalAdvance(m_title), valueFm.horizontalAdvance(m_value));
    if (!m_subtitle.isEmpty()) {
        textWidth = qMax(textWidth, titleFm.horizontalAdvance(m_subtitle));
    }

    int totalHeight = padY * 2 + titleFm.height() + gap + valueFm.height();
    if (!m_subtitle.isEmpty()) {
        totalHeight += gap + titleFm.height();
    }

    return {textWidth + padX * 2, totalHeight};
}

QSize StatCard::minimumSizeHint() const {
    return sizeHint();
}

void StatCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto tokens = m_theme.currentTokens();
    const QRectF cardRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // Card background and border
    painter.setPen(QPen(tokens.border, 1.0));
    painter.setBrush(tokens.surface);
    painter.drawRoundedRect(cardRect, kCardRadius, kCardRadius);

    const QFontMetrics titleFm(font());
    QFont valFont = font();
    valFont.setPointSizeF(font().pointSizeF() * 1.8);
    valFont.setBold(true);
    const QFontMetrics valueFm(valFont);

    const int padX = qMax(16, titleFm.height());
    const int padY = qMax(14, static_cast<int>(titleFm.height() * 0.9));
    const int gap = qMax(4, titleFm.height() / 4);

    // Title
    painter.setFont(font());
    painter.setPen(tokens.textMuted);
    const QRect titleRect(padX, padY, width() - padX * 2, titleFm.height());
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // Value
    painter.setFont(valFont);
    painter.setPen(tokens.text);
    const int valueTop = padY + titleFm.height() + gap;
    const QRect valueRect(padX, valueTop, width() - padX * 2, valueFm.height());
    painter.drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter, m_value);

    // Subtitle (if any)
    if (!m_subtitle.isEmpty()) {
        painter.setFont(font());
        painter.setPen(tokens.textMuted);
        const int subTop = valueTop + valueFm.height() + gap;
        const QRect subRect(padX, subTop, width() - padX * 2, titleFm.height());
        painter.drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter, m_subtitle);
    }
}

}  // namespace JobPrep::Ui::Widgets
