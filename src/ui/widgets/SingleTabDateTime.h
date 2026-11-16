#pragma once

#include <QApplication>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSettings>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QTableView>
#include <QTimeEdit>
#include <QTimer>

namespace JobPrep::Ui::Widgets {

/// Returns true if the key and modifier match the calendar popup shortcuts (Alt+Down or F4, keypad tolerated).
inline bool isCalendarPopupShortcut(int key, Qt::KeyboardModifiers modifiers) {
    const auto mods = modifiers & ~Qt::KeypadModifier;
    return (mods == Qt::AltModifier && key == Qt::Key_Down) ||
           (mods == Qt::NoModifier && key == Qt::Key_F4);
}

/// A QDateEdit where Tab and Backtab advance to the next or previous widget
/// on a single key press instead of stepping through internal date sections,
/// and Alt+Down / F4 opens the calendar popup.
class SingleTabDateEdit : public QDateEdit {
public:
    static QString tr(const char* sourceText, const char* disambiguation = nullptr, int n = -1) {
        return QCoreApplication::translate("SingleTabDateEdit", sourceText, disambiguation, n);
    }

    explicit SingleTabDateEdit(QWidget* parent = nullptr)
        : QDateEdit(parent) {
        setDisplayFormat(QString::fromLatin1("yyyy-MM-dd"));
        initShortcutHint();
        if (calendarPopup()) {
            setupCalendarPopupFocus();
        }
        if (lineEdit()) {
            lineEdit()->installEventFilter(this);
        }
    }

    explicit SingleTabDateEdit(const QDate& date, QWidget* parent = nullptr)
        : QDateEdit(date, parent) {
        setDisplayFormat(QString::fromLatin1("yyyy-MM-dd"));
        initShortcutHint();
        if (calendarPopup()) {
            setupCalendarPopupFocus();
        }
        if (lineEdit()) {
            lineEdit()->installEventFilter(this);
        }
    }

    void syncCalendarFirstDayOfWeek() {
        if (auto* cal = calendarWidget()) {
            QSettings s;
            const int dayInt = s.value(QString::fromLatin1("calendar/firstDayOfWeek"), 1).toInt();
            cal->setFirstDayOfWeek((dayInt == 7) ? Qt::Sunday : Qt::Monday);
        }
    }

    void setupCalendarPopupFocus() {
        syncCalendarFirstDayOfWeek();
        if (auto* cal = calendarWidget()) {
            cal->setFocusPolicy(Qt::StrongFocus);
            if (auto* calTable = cal->findChild<QTableView*>()) {
                calTable->setFocusPolicy(Qt::StrongFocus);
                cal->setFocusProxy(calTable);
            }
            cal->installEventFilter(this);
            if (auto* win = cal->window()) {
                win->installEventFilter(this);
            }
        }
    }

    void setCalendarPopup(bool enable) {
        QDateEdit::setCalendarPopup(enable);
        if (enable) {
            setupCalendarPopupFocus();
        }
    }

    void setToolTip(const QString& tip) {
        const QString hint =
            QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
        if (tip.isEmpty()) {
            QDateEdit::setToolTip(hint);
        } else if (tip.contains(hint)) {
            QDateEdit::setToolTip(tip);
        } else {
            QDateEdit::setToolTip(tip + QString(u'\n') + hint);
        }
    }

    void focusCalendarView() {
        if (auto* cal = calendarWidget()) {
            auto* calTable = cal->findChild<QTableView*>();
            QWidget* target = calTable ? static_cast<QWidget*>(calTable) : static_cast<QWidget*>(cal);
            target->setFocus(Qt::PopupFocusReason);
            QTimer::singleShot(0, target, [this, target]() {
                if (calendarWidget() && calendarWidget()->isVisible()) {
                    target->setFocus(Qt::PopupFocusReason);
                    if (qEnvironmentVariableIntValue("JOBPREP_DEBUG_DATEPOPUP") == 1) {
                        logDatePopupDebug();
                    }
                }
            });
        }
    }

    void logDatePopupDebug() const {
        QWidget* popup = calendarWidget() ? calendarWidget()->window() : nullptr;
        QWidget* fw = QApplication::focusWidget();
        fprintf(stderr, "JOBPREP_DEBUG_DATEPOPUP: popup=%p class=%s flags=0x%x visible=%d\n",
                static_cast<void*>(popup),
                popup ? popup->metaObject()->className() : "null",
                popup ? static_cast<unsigned int>(popup->windowFlags()) : 0,
                popup ? popup->isVisible() : 0);
        fprintf(stderr, "JOBPREP_DEBUG_DATEPOPUP: focusWidget=%p class=%s name=%s\n",
                static_cast<void*>(fw),
                fw ? fw->metaObject()->className() : "null",
                fw ? qPrintable(fw->objectName()) : "");
        int depth = 0;
        for (QWidget* w = fw; w != nullptr; w = w->parentWidget()) {
            fprintf(stderr, "JOBPREP_DEBUG_DATEPOPUP:   chain[%d]: %p class=%s name=%s\n",
                    depth++, static_cast<void*>(w), w->metaObject()->className(), qPrintable(w->objectName()));
        }
    }

    void openCalendarPopup() {
        if (!calendarPopup()) return;
        setupCalendarPopupFocus();
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
        focusCalendarView();
    }

protected:
    bool focusNextPrevChild(bool next) override {
        return QWidget::focusNextPrevChild(next);
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (auto* cal = calendarWidget()) {
            if (watched == cal || watched == cal->window()) {
                if (event->type() == QEvent::Show) {
                    focusCalendarView();
                }
            }
        }
        if (watched == lineEdit()) {
            if (event->type() == QEvent::ShortcutOverride) {
                auto* ke = static_cast<QKeyEvent*>(event);
                if (isCalendarPopupShortcut(ke->key(), ke->modifiers())) {
                    ke->accept();
                    return true;
                }
            } else if (event->type() == QEvent::KeyPress) {
                auto* ke = static_cast<QKeyEvent*>(event);
                if (isCalendarPopupShortcut(ke->key(), ke->modifiers())) {
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
            const QString hint =
                QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
            const QString cur = toolTip();
            if (!cur.contains(hint)) {
                const QString combined = cur.isEmpty() ? hint : (cur + u'\n' + hint);
                QDateEdit::setToolTip(combined);
            }
        }
        if (e->type() == QEvent::ShortcutOverride) {
            auto* ke = static_cast<QKeyEvent*>(e);
            if (isCalendarPopupShortcut(ke->key(), ke->modifiers())) {
                ke->accept();
                return true;
            }
        }
        return QDateEdit::event(e);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (isCalendarPopupShortcut(event->key(), event->modifiers())) {
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
