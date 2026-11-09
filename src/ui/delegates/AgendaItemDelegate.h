#pragma once

#include <QRect>
#include <QString>
#include <QStyledItemDelegate>

class QModelIndex;

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Delegates {

/// Layout geometry and formatted text calculated for an agenda item row.
struct AgendaItemLayout {
    QRect iconRect;
    QRect titleRect;
    QRect subtitleRect;
    QRect rightTextRect;
    int rowHeight{0};
    bool hasSubtitle{false};
    QString elidedTitle;
    QString elidedSubtitle;
    QString rightText;
};

/// Paints one agenda row: a kind-colored icon, the item title, a muted subtitle and the
/// time on the right. Past interviews are dimmed; overdue follow-ups and study targets
/// keep their attention color (SPEC §3.4 C6, Part B report).
class AgendaItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit AgendaItemDelegate(JobPrep::Ui::Theme::ThemeManager& theme, QObject* parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    static AgendaItemLayout computeLayout(const QStyleOptionViewItem& option,
                                          const QModelIndex& index);

private:
    JobPrep::Ui::Theme::ThemeManager& m_theme;
};

}  // namespace JobPrep::Ui::Delegates