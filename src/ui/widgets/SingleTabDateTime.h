#pragma once

#include <QDateEdit>
#include <QTimeEdit>

namespace JobPrep::Ui::Widgets {

/// A QDateEdit where Tab and Backtab advance to the next or previous widget
/// on a single key press instead of stepping through internal date sections.
class SingleTabDateEdit : public QDateEdit {
public:
    using QDateEdit::QDateEdit;

protected:
    bool focusNextPrevChild(bool next) override {
        return QWidget::focusNextPrevChild(next);
    }
};

/// A QTimeEdit where Tab and Backtab advance to the next or previous widget
/// on a single key press instead of stepping through internal time sections.
class SingleTabTimeEdit : public QTimeEdit {
public:
    using QTimeEdit::QTimeEdit;

protected:
    bool focusNextPrevChild(bool next) override {
        return QWidget::focusNextPrevChild(next);
    }
};

}  // namespace JobPrep::Ui::Widgets
