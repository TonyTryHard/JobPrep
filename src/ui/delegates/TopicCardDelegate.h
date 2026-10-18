#pragma once

#include <QStyledItemDelegate>

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Delegates {

/// Kanban card for the Study board (SPEC §3.2 S5): track colour stripe, title,
/// progress bar, due chip (red only when OverdueRole is true) and a priority
/// marker that is readable without colour. Card height derives from font metrics.
class TopicCardDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit TopicCardDelegate(Theme::ThemeManager& theme, QObject* parent = nullptr);
    ~TopicCardDelegate() override = default;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    Theme::ThemeManager& m_theme;
};

}  // namespace JobPrep::Ui::Delegates
