#pragma once

#include <QApplication>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QTimeEdit>

namespace JobPrep::Ui::Widgets {

/// A QDateEdit where Tab and Backtab advance to the next or previous widget
/// on a single key press instead of stepping through internal date sections,
/// and Alt+Down / F4 opens the calendar popup.
class SingleTabDateEdit : public QDateEdit {
public:
    explicit SingleTabDateEdit(QWidget* parent = nullptr)
        : QDateEdit(parent) {
        initShortcutHint();
        if (lineEdit()) {
            lineEdit()->installEventFilter(this);
        }
    }

    explicit SingleTabDateEdit(const QDate& date, QWidget* parent = nullptr)
        : QDateEdit(date, parent) {
        initShortcutHint();
        if (lineEdit()) {
            lineEdit()->installEventFilter(this);
        }
    }

    void setToolTip(const QString& tip) {
        const QString hint = tr("Press Alt+Down or F4 to open calendar");
        if (tip.isEmpty()) {
            QDateEdit::setToolTip(hint);
        } else if (tip.contains(hint)) {
            QDateEdit::setToolTip(tip);
        } else {
            QDateEdit::setToolTip(tip + QString(u'\n') + hint);
        }
    }

    void openCalendarPopup() {
        if (!calendarPopup()) return;
        if (calendarWidget() && calendarWidget()->isVisible()) return;

        QStyleOptionComboBox opt;
        opt.initFrom(this);
        opt.rect = rect();
        opt.subControls = QStyle::SC_All;
        opt.activeSubControls = QStyle::SC_None;
        opt.state |= QStyle::State_Enabled;
        const QRect arrowRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this);
        const QPoint clickPos = (arrowRect.isValid() && !arrowRect.isEmpty()) ? arrowRect.center() : QPoint(width() - 10, height() / 2);

        QMouseEvent press(QEvent::MouseButtonPress, clickPos, mapToGlobal(clickPos),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(this, &press);
    }

protected:
    bool focusNextPrevChild(bool next) override {
        return QWidget::focusNextPrevChild(next);
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == lineEdit()) {
            if (event->type() == QEvent::ShortcutOverride) {
                auto* ke = static_cast<QKeyEvent*>(event);
                const auto mods = ke->modifiers() & ~Qt::KeypadModifier;
                if ((mods == Qt::AltModifier && ke->key() == Qt::Key_Down) ||
                    (mods == Qt::NoModifier && ke->key() == Qt::Key_F4)) {
                    ke->accept();
                    return true;
                }
            } else if (event->type() == QEvent::KeyPress) {
                auto* ke = static_cast<QKeyEvent*>(event);
                const auto mods = ke->modifiers() & ~Qt::KeypadModifier;
                if ((mods == Qt::AltModifier && ke->key() == Qt::Key_Down) ||
                    (mods == Qt::NoModifier && ke->key() == Qt::Key_F4)) {
                    openCalendarPopup();
                    ke->accept();
                    return true;
                }
            }
        }
        return QDateEdit::eventFilter(watched, event);
    }

    bool event(QEvent* e) override {
        if (e->type() == QEvent::ToolTipChange) {
            const QString hint = tr("Press Alt+Down or F4 to open calendar");
            const QString cur = toolTip();
            if (!cur.contains(hint)) {
                const QString combined = cur.isEmpty() ? hint : (cur + u'\n' + hint);
                QDateEdit::setToolTip(combined);
            }
        }
        if (e->type() == QEvent::ShortcutOverride) {
            auto* ke = static_cast<QKeyEvent*>(e);
            const auto mods = ke->modifiers() & ~Qt::KeypadModifier;
            if ((mods == Qt::AltModifier && ke->key() == Qt::Key_Down) ||
                (mods == Qt::NoModifier && ke->key() == Qt::Key_F4)) {
                ke->accept();
                return true;
            }
        }
        return QDateEdit::event(e);
    }

    void keyPressEvent(QKeyEvent* event) override {
        const auto mods = event->modifiers() & ~Qt::KeypadModifier;
        if ((mods == Qt::AltModifier && event->key() == Qt::Key_Down) ||
            (mods == Qt::NoModifier && event->key() == Qt::Key_F4)) {
            openCalendarPopup();
            event->accept();
            return;
        }
        QDateEdit::keyPressEvent(event);
    }

private:
    void initShortcutHint() {
        setToolTip(toolTip());
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
