#pragma once

#include <QStyledItemDelegate>

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Delegates {

/// Progress bar delegate for table views (SPEC §3.2 S6, C4).
/// Renders a themed horizontal progress bar with percentage text.
class ProgressBarDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ProgressBarDelegate(Theme::ThemeManager& theme, QObject* parent = nullptr);
    ~ProgressBarDelegate() override = default;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    Theme::ThemeManager& m_theme;
};

}  // namespace JobPrep::Ui::Delegates
