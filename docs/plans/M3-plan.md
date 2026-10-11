# M3 — Calendar & interviews

Status: written, waiting for approval before any code change.
Three commits (Part A, Part B, Part C), each only after `./scripts/check.sh` passes. Local commits only, never push.

Environment: Linux, Qt 6.10.2 (system, `/usr/lib/x86_64-linux-gnu/cmake/Qt6`), Ninja / Debug preset `dev-linux`, `JOBPREP_WERROR=ON`. Build with `CMAKE_BUILD_PARALLEL_LEVEL=2`. A live display is available (X11 `:0` + Wayland session).

---

## 0. Pre-flight findings (verified against the real code)

| # | Question | Finding |
|---|---|---|
| 1a | **Wishlist `applied_date` on insert** | `insertRow()` calls `appliedDateFor(status, appliedDate)`, which returns `appliedDate` unchanged whenever `status != Applied`. A wishlist application inserted with `appliedDate = nullopt` stores NULL. **Correct.** |
| 1b | **`setStatus` wishlist → applied, empty date** | `updateStatusRow()` calls `appliedDateFor(status, stored->appliedDate)`, which fills today when status is `Applied` and `appliedDate` is empty. **Correct.** |
| 1c | **Test coverage** | `tst_ApplicationRepository` has both `insertAppliesTodayForAppliedStatus` (direct insert, `Applied`, no date → today) and `setStatusAppliesTodayForAppliedStatus` (wishlist → `Applied` fills today; existing date never overwritten). **Correct; no fix needed.** |
| 2 | **Connection isolation regression test** | `tst_Database` has no test for two live `Database` objects staying independent after one is destroyed. The code itself is correct (UUID connection name introduced in M2), but there is no test. **Add one in Part A.** |
| 3 | **`ui_smoke` message filter** | `isPlatformPluginNoise()` checks `msg.contains(u"does not support propagateSizeHints"_s)`. This is broad (matches any platform plugin noise with that substring, not just the offscreen one). The offscreen platform always runs the tests, and the message string is unique enough that it cannot collide with app messages. The filter is acceptable. **No change needed.** |
| 4a | **ChipBar** | `ChipBar` uses an exclusive `QButtonGroup` — it is **single-select only**, not independently toggleable. The calendar layer chips (Interviews / Follow-ups / Study targets) must each be independently toggleable. The calendar page will use three `QPushButton` (checkable, non-exclusive) instead of `ChipBar`, or a new `ToggleChipBar` variant. Default: **three standalone `QPushButton` (checkable, no group)** to avoid forking the M2 widget. |
| 4b | **`firstDayOfWeek` key** | `SettingsService` already has `firstDayOfWeek()` / `setFirstDayOfWeek(int)` backed by `calendar/firstDayOfWeek` (default 1 = Monday). **Reusable.** |
| 4c | **`PageBase` hooks** | `focusSearch()` and `triggerNew()` exist with default no-op implementations. **Reusable.** |
| 4d | **`StatusStyle` / `Tokens`** | `StatusStyle` provides per-status color and label; `Tokens`/`ColorTokens` provides all surface, border, and accent colors. `AgendaItem` currently carries a `QColor color` field. Q4 says keep `QColor` out of `domain`; the field must be removed from the struct and left to the UI to map via `StatusStyle`/`Tokens`. **Fix in Part A.** |
| 4e | **`InterviewRepository` range query** | `inRange(QDateTime from, QDateTime to)` already exists. **Reusable.** |
| 4f | **`ApplicationRepository` follow-up queries** | No `byNextActionDate(QDate from, QDate to)` query exists yet. `AgendaService` will call `all()` and filter in memory for v1 (applications table expected small, < a few thousand rows). **Acceptable for v1; add `byNextActionDateRange` to Part A for correctness and testability.** |
| 4g | **`TopicRepository` target date queries** | No range query by `target_date` exists yet. Same strategy: `AgendaService` calls `all()` + in-memory filter. **Add `byTargetDateRange` to Part A.** |
| 4h | **Jobs "Next step" column refresh on interview change** | `ApplicationTableModel` reloads on `ApplicationRepository::changed()` only; it does not observe `InterviewRepository::changed()`. The "Next step" column for a row shows the next upcoming interview (computed in the model). After M3 adds interviews, this refresh gap will be visible. **Add the connection in Part A (one line in `ApplicationTableModel::ApplicationTableModel`).**  |
| 4i | **`AgendaItem.color` in domain** | `Structs.h` already defines `AgendaItem` with a `QColor color` field and `#include <QColor>`. `QColor` is a `QtGui` type and must not be in `domain`. **Remove `color` field and the `<QColor>` include from `Structs.h`; UI maps `kind` to a color using `Tokens`. Fix in Part A.** |

---

## Design decisions

| Q | Decision | Default | Reason |
|---|---|---|---|
| Q1 | Which interviews show | All except `Cancelled`; past ones drawn dimmed; outcome shown in subtitle | Cancelled are noise; outcome helps at a glance |
| Q2 | Follow-up semantics | One item per application with `next_action_date` set and status not closed (not rejected/withdrawn/ghosted/accepted); wishlist included. "＋ Follow-up" asks for application and action text, sets selected date as `next_action_date`, confirms before replacing | Simplest interpretation of SPEC C4 |
| Q3 | Study targets | Topics with `target_date` set and `status != done` | M4 brings the Study page; clicking opens a tooltip "Opens in M4" |
| Q4 | `AgendaItem` and color | `AgendaItem.color` removed from domain; `AgendaKind` maps to a `Tokens` color in the UI layer | `QColor` must not enter `domain` (Qt Gui dependency) |
| Q5 | Ordering and day assignment | Interview → day of `start_at`; follow-up → day of `next_action_date`; target → day of `target_date`. Order within a day: interviews by time, then follow-ups, then study targets, ties by title | Deterministic, spec-consistent |
| Q6 | Status side effects | Creating an interview does not change the application status automatically | Mentioned as a future option in the dialog (tooltip or comment only) |
| Q7 | Layer toggles persistence | Stored in `SettingsService` under `calendar/layerInterviews`, `calendar/layerFollowUps`, `calendar/layerTargets` (default true). Accessors added to `SettingsService` in Part A | Per-session state would be annoying to lose on restart |
| Q8 | Clicking an agenda item | Interview → `InterviewDialog` (edit mode); follow-up → `ApplicationDialog` (Overview tab); study target → no-op, tooltip "Opens in M4" | Spec C8, Q3 |
| Q9 | Transactions | `InterviewDialog` only touches `InterviewRepository` → no outer transaction needed for the dialog itself; the "＋ Follow-up" path updates `ApplicationRepository.setNextAction` in one call, so no outer transaction needed there either | Single-repo writes don't need a manually composed transaction |

---

## Part split rationale

Three commits instead of the suggested two, because Part B (the month grid) is the largest single piece of new custom painting and deserves its own green checkpoint before the glue (Part C) is added.

| Part | Commit tag | What it delivers |
|---|---|---|
| **A** | `fix(data): pre-flight fixes + AgendaService` | Connection-isolation test, `AgendaItem.color` removal, `byNextActionDateRange`, `byTargetDateRange`, layer-toggle keys in `SettingsService`, `ApplicationTableModel` refresh on `InterviewRepository::changed()`, `AgendaService` + full test suite |
| **B** | `feat(ui): InterviewDialog + editable Interviews tab + Jobs wiring` | `InterviewDialog`, editable Interviews tab in `ApplicationDialog`, Jobs context menu "Add interview…" enabled, `+ New → Interview` enabled and routed via `CalendarPage::triggerNew()` |
| **C** | `feat(ui): MonthView + AgendaListModel + CalendarPage` | `MonthView`, `AgendaListModel`, `CalendarPage` (month + layer chips + agenda panel + ＋Follow-up), keyboard nav, `ui_smoke` extensions |

---

## Part A — Pre-flight fixes, new repository queries, AgendaService

### Files touched / added

| File | Status | Purpose |
|---|---|---|
| `tests/data/tst_Database.cpp` | modify | Add `twoLiveDatabasesAreIndependent` test slot |
| `src/domain/Structs.h` | modify | Remove `QColor color` field and `<QColor>` include from `AgendaItem` |
| `src/data/ApplicationRepository.h` | modify | Add `byNextActionDateRange(QDate, QDate)` declaration |
| `src/data/ApplicationRepository.cpp` | modify | Implement `byNextActionDateRange` |
| `src/data/TopicRepository.h` | modify | Add `byTargetDateRange(QDate, QDate)` declaration |
| `src/data/TopicRepository.cpp` | modify | Implement `byTargetDateRange` (non-done topics with target_date in range) |
| `src/models/ApplicationTableModel.cpp` | modify | Connect to `InterviewRepository::changed()` in constructor so "Next step" refreshes |
| `src/models/ApplicationTableModel.h` | modify | Store `InterviewRepository&` member; update constructor signature |
| `src/services/SettingsService.h` | modify | Add `calendarLayerInterviews/FollowUps/Targets` accessors |
| `src/services/SettingsService.cpp` | modify | Implement the three new accessors (key names above, default true) |
| `src/services/AgendaService.h` | **new** | `AgendaService` class; depends on all three repos |
| `src/services/AgendaService.cpp` | **new** | Implementation of `items(QDate from, QDate to)` |
| `tests/services/tst_AgendaService.cpp` | **new** | Full test suite (see test table) |
| `CMakeLists.txt` | modify | Add `AgendaService`, `tst_AgendaService` targets; update `ApplicationTableModel` to include `InterviewRepository` |

### `AgendaService` design

```cpp
// src/services/AgendaService.h
namespace JobPrep::Services {

/// Merges interviews, follow-ups and study targets into a flat, sorted list
/// of AgendaItem values for a date range (SPEC §3.4 C6).
class AgendaService {
public:
    explicit AgendaService(
        Data::InterviewRepository& interviews,
        Data::ApplicationRepository& apps,
        Data::TopicRepository& topics);

    /// Returns items whose date falls in [from, to] inclusive, sorted by
    /// (date, kind, time, title). kind order: Interview < FollowUp < StudyTarget.
    QList<Domain::AgendaItem> items(QDate from, QDate to) const;
};
}
```

`AgendaItem` carries: `kind`, `date`, `time` (optional — interviews have it, follow-ups and targets do not), `title` (e.g. "Technical · Globex"), `subtitle` (e.g. "Zoom · Anna K." or company+position), `refId` (interview id, application id, or topic id). No `QColor`.

**Follow-up filter:** `next_action_date` in range AND status not in {rejected, withdrawn, ghosted, accepted}. Wishlist is included.

**Study target filter:** `target_date` in range AND `status != done`.

**Interview filter:** start date in range AND `outcome != cancelled`.

### Part A test table

| Test slot (`tst_AgendaService`) | What it covers |
|---|---|
| `noItemsForEmptyDatabase` | Empty result when DB has no data |
| `interviewAppearsOnItsStartDay` | One interview → exactly one item on start date |
| `cancelledInterviewExcluded` | `outcome == Cancelled` → not returned |
| `pastInterviewIncluded` | Past date, non-cancelled → still returned |
| `followUpForActiveApplication` | `next_action_date` set, status active → one item |
| `followUpForWishlistApplication` | wishlist status → item returned |
| `followUpExcludedForClosedApplication` | rejected/withdrawn/ghosted/accepted → not returned |
| `studyTargetForOpenTopic` | target_date set, backlog → item returned |
| `doneTopicExcluded` | status done → not returned |
| `rangeAcrossMonthBorder` | 31 Oct to 1 Nov: items on both days returned |
| `rangeAcrossYearEnd` | 31 Dec to 1 Jan: items on both days returned |
| `leapDayMonth` | range covering 29 Feb 2028: item on 29 Feb returned |
| `lateNightInterview` | 23:30 start on date D: falls on day D not D+1 |
| `deterministic ordering` | Two interviews, one follow-up on same day → interview (by time) first, then follow-up |
| `singleDayRange` | `from == to` returns only items on that day |
| **`tst_Database::twoLiveDatabasesAreIndependent`** | Two `TestDatabase` instances both open `:memory:` concurrently; destroy first; second's `SELECT 1` still works |

---

## Part B — InterviewDialog, editable Interviews tab, Jobs wiring

### Files touched / added

| File | Status | Purpose |
|---|---|---|
| `src/domain/Enums.h` | check | Verify `InterviewType` and `InterviewOutcome` enums present (they are) |
| `src/ui/dialogs/InterviewDialog.h` | **new** | Dialog to create/edit one interview (SPEC §3.4) |
| `src/ui/dialogs/InterviewDialog.cpp` | **new** | Implementation (AUTOMOC) |
| `src/ui/dialogs/ApplicationDialog.h` | modify | Make Interviews tab editable; add interview add/edit/delete actions |
| `src/ui/dialogs/ApplicationDialog.cpp` | modify | Interviews tab: table + Add/Edit/Delete buttons; double-click edits |
| `src/ui/pages/JobsPage.cpp` | modify | Enable "Add interview…" context menu action; connect to `openAddInterview()` |
| `src/ui/pages/JobsPage.h` | modify | Add `openAddInterview(int appId)` private slot |
| `src/ui/MainWindow.cpp` | modify | Enable `HeaderNewInterview` action; connect to `CalendarPage::triggerNew()` |
| `src/ui/pages/CalendarPage.h` | modify | Add `triggerNew()` override; add `AppContext&` injection |
| `src/ui/pages/CalendarPage.cpp` | modify | `triggerNew()` opens `InterviewDialog(no app locked, today)` |
| `src/app/AppContext.h` | check | `interviews()` already exposed |
| `CMakeLists.txt` | modify | Add `InterviewDialog` sources |

### `InterviewDialog` design

Fields (SPEC §3.4 scope item 1):
- **Application** combo (lists all, search-filtered `QComboBox`; locked when opened from an `ApplicationDialog` or `JobsPage` with a known `appId`)
- **Date** (`QDateEdit`, default today or prefilled)
- **Time** (`QTimeEdit`, default 10:00)
- **Duration** (`QSpinBox` minutes, min 1, default 60)
- **Type** (`QComboBox`: HR / Technical / Live coding / System design / Behavioral / Final / Other)
- **Place** (`QLineEdit`, link or address)
- **Interviewer** (`QLineEdit`)
- **Notes** (`QTextEdit`)
- **Outcome** (`QComboBox`: Pending / Passed / Failed / Cancelled; default Pending)
- Inline validation: application required, valid date and time, duration ≥ 1.
- Dirty tracking; "Discard changes?" on Cancel/close when dirty.
- Edit mode: **Delete** button with confirmation (bottom-left of button row).
- Object names: `FieldApplication`, `FieldDate`, `FieldTime`, `FieldDuration`, `FieldType`, `FieldPlace`, `FieldInterviewer`, `FieldNotes`, `FieldOutcome`, `DialogSaveBtn`, `DialogDeleteBtn`.

```cpp
class InterviewDialog : public QDialog {
    enum Mode { Create, Edit };
    explicit InterviewDialog(
        Data::ApplicationRepository& apps,
        Data::InterviewRepository& interviews,
        Mode mode,
        QWidget* parent = nullptr);
    void setInterview(const Domain::Interview& interview);  // Edit mode
    void setApplicationId(int appId);                       // lock combo
    void setDate(QDate date);                               // prefill from calendar
    Domain::Interview interview() const;
    bool isDirty() const;
    void reject() override;
};
```

### Editable Interviews tab in `ApplicationDialog`

The existing read-only `QTableWidget` (`InterviewsTable`) becomes:
- A table with columns: Date/Time, Type, Place, Outcome.
- Three buttons below (or toolbar above): **Add**, **Edit**, **Delete** (Delete grayed when no row selected).
- Double-click → opens `InterviewDialog` in Edit mode.
- Add → opens `InterviewDialog` in Create mode with the application locked.
- The "save first" hint label is shown and the buttons disabled when in Create mode (application not yet saved).
- After every interview write the table reloads from `InterviewRepository::byApplication(id)`.
- Still disabled for a brand-new application (same as Timeline tab).

### Jobs page wiring

- Context menu "Add interview…" is enabled and calls a new private slot `openAddInterview(int appId)` that opens `InterviewDialog(Create, appId locked)`.
- `MainWindow::setupNewMenu`: enable `HeaderNewInterview`; connect to `m_calendarPage->triggerNew()`.
- `CalendarPage::triggerNew()`: opens `InterviewDialog(Create, no appId, date = today)`.
- `CalendarPage` must receive an `AppContext&` in its constructor (currently it takes no context); `MainWindow` updated.
- `+ New → Study session` stays disabled with tooltip "Arrives in M4".

### Part B test additions (`ui_smoke`, slots before `testZeroWarnings`)

| Slot | Covers |
|---|---|
| `testInterviewDialogCreateAndEdit` | Create dialog: fill, save → `InterviewRepository::all().size() == 1`. Edit dialog: prefilled from stored, change outcome, save → stored. Dirty tracking: change a field → `isDirty()`; click Cancel → discard confirmation. |
| `testInterviewDialogValidation` | Application required, duration ≥ 1. |
| `testInterviewDialogDeleteWithConfirmation` | Edit mode → Delete button present; confirm → interview gone; cancel confirmation → interview kept. |
| `testApplicationDialogInterviewsTabEditable` | Existing app → Interviews tab enabled; Add opens dialog; interview appears in table. New app → tab disabled with hint. |
| `testJobsContextMenuAddInterview` | Right-click row → "Add interview…" enabled; triggers dialog (one `InterviewDialog` top-level). |
| `testHeaderNewInterviewEnabled` | `HeaderNewInterview` action is now enabled; triggering it opens one `InterviewDialog`. |
| `testInterviewCreatedFromJobsRefreshesNextStep` | Insert interview via `InterviewRepository`; "Next step" column in `ApplicationTableModel` updates without restarting. |

---

## Part C — MonthView, AgendaListModel, CalendarPage

### Files touched / added

| File | Status | Purpose |
|---|---|---|
| `src/ui/widgets/MonthView.h` | **new** | Custom-painted month grid widget |
| `src/ui/widgets/MonthView.cpp` | **new** | Implementation (AUTOMOC) |
| `src/models/AgendaListModel.h` | **new** | `QAbstractListModel` for the selected-day agenda panel |
| `src/models/AgendaListModel.cpp` | **new** | Implementation (AUTOMOC) |
| `src/ui/pages/CalendarPage.h` | modify | Full calendar page with all fields |
| `src/ui/pages/CalendarPage.cpp` | modify | Connect all pieces |
| `src/services/AgendaService.h` | modify | `AgendaService` already added in A; exposed via `AppContext` |
| `src/app/AppContext.h` | modify | Add `AgendaService& agendaService()` |
| `src/app/AppContext.cpp` | modify | Construct `AgendaService` |
| `CMakeLists.txt` | modify | Add `MonthView`, `AgendaListModel` sources |

### `MonthView` widget design

```cpp
class MonthView : public QWidget {
    Q_OBJECT
public:
    explicit MonthView(QWidget* parent = nullptr);
    void setFirstDayOfWeek(Qt::DayOfWeek day);    // Monday or Sunday
    void setDisplayMonth(QDate month);             // any day of the month
    void setSelectedDay(QDate day);
    void setMarkers(const QHash<QDate, QList<QColor>>& markers); // ≤ 3 dots per day
    QDate selectedDay() const;
    QDate displayMonth() const;
signals:
    void daySelected(QDate day);
    void dayActivated(QDate day);   // double-click
    void monthChanged(QDate month);  // emitted on prev/next/today
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
};
```

**Painting rules:**
- Title row: "← October 2026 →" centered, Today button right-aligned — painted as clickable rectangles, not separate QWidgets. This avoids parented-child layout drift and keeps paint entirely in `paintEvent`.
- 6 × 7 grid below the header. One extra row is always painted (6 rows × 7 cols = 42 cells) to avoid layout jumps.
- Today cell: accent-colored background.
- Selected cell: outlined with `Tokens.accent`, 2 px border.
- Out-of-month cells: `Tokens.textMuted` for the day number.
- Up to 3 colored dots centered below the day number; overflow "＋N" label in `textMuted`.
- Uses `devicePixelRatio()` via `painter.device()->devicePixelRatio()` for HiDPI-correct line widths.
- No per-paint heap allocations for the markers (markers pre-cached in a `QHash`).
- Colors from `ColorTokens` only; no hard-coded values.

**Keyboard navigation:**
- Left/Right → prev/next day.
- Up/Down → same weekday ± 7 days.
- PageUp/PageDown → prev/next month, same day of month (clamped to last day).
- Home → today.
- Enter / Return → emits `dayActivated`.

### `AgendaListModel`

- `QAbstractListModel` with roles: `DateTimeRole`, `KindRole`, `TitleRole`, `SubtitleRole`, `RefIdRole`, `ColorRole` (QColor, mapped by kind in the model from `Tokens`).
- `setItems(QList<Domain::AgendaItem>)` replaces the list and calls `reset()`.
- Used by a `QListView` in the agenda panel.

### `CalendarPage` layout

```
+--[ MonthView ]------------------+--[ Agenda panel ]--+
|  ← Oct 2026 →  [Today]         | Fri, 2 Oct          |
|  [Interviews][Follow-ups][Study]|  14:00 Technical…   |
|  Mo Tu We Th Fr Sa Su          |  Follow-up · Acme   |
|  28 29 30  1  2  3  4          |                     |
|             ●●                 |  [+ Interview]       |
|   5  6 ...                     |  [+ Follow-up]       |
+----------------------------------+---------------------+
```

- `QHBoxLayout` with the `MonthView` on the left (stretch 1) and a `QWidget` agenda panel on the right (fixed ~300 px).
- Layer chips: three `QPushButton` (checkable, not in a group) for Interviews, Follow-ups, Study targets. Initial state from `SettingsService`; toggled state written back on each click.
- Agenda panel: `QLabel` for the date, a `QListView` backed by `AgendaListModel`, "＋ Interview" and "＋ Follow-up" buttons.
- `QListView` uses a custom delegate (`AgendaItemDelegate`) that paints kind icon (monochrome SVG tinted with the item's color), time (if any), title (semibold), subtitle (textMuted).
- Empty day: `EmptyState` widget swapped in via a `QStackedWidget`.
- "＋ Follow-up" opens a small `QDialog` (not a full dialog): app combo, action text, date (prefilled with selected day). Confirms before replacing existing `next_action`.

**Reload triggers:**
- `InterviewRepository::changed()` → reload markers for current month + agenda for selected day.
- `ApplicationRepository::changed()` → same.
- `TopicRepository::changed()` → same.
- `SettingsService::firstDayOfWeekChanged()` → rebuild grid via `MonthView::setFirstDayOfWeek()`.
- `onActivated()` → reload markers + agenda (cheap: range query on a small table).

**Clicking an agenda item:**
- Interview → open `InterviewDialog(Edit, id)`.
- Follow-up → open `ApplicationDialog(Edit, appId)`, switch to Overview tab (tab index 0).
- Study target → no-op with a status-bar tooltip "Opens in M4".

### Part C test additions

**`tests/ui_smoke/tst_AgendaListModel.cpp`** — new Qt Test target `tst_AgendaListModel`:

| Slot | Covers |
|---|---|
| `emptyModelHasNoRows` | `setItems({})` → rowCount 0 |
| `setItemsFilledRows` | 3 items → rowCount 3; `TitleRole` data matches |
| `colorRoleMapsByKind` | Interview → accent/orange; follow-up → blue; target → green |
| `resetReplacesList` | setItems twice → second wins |

**`tests/ui_smoke/tst_MonthView.cpp`** — new Qt Test target `tst_MonthView`:

| Slot | Covers |
|---|---|
| `mondayFirstGridStartsMonday` | First column header is Monday |
| `sundayFirstGridStartsSunday` | First column header is Sunday |
| `monthNeedingSixRows` | October 2023 starts on Sun and needs 6 rows in Mon-first mode |
| `todayHighlightedOnCurrentMonth` | `selectedDay()` defaults to today; today is in the month |
| `selectionChangedOnClick` | Simulated mouse click → `daySelected` emitted, `selectedDay()` updated |
| `keyNavigationArrows` | Left/Right/Up/Down update `selectedDay()` |
| `keyNavigationPageUpDown` | PageUp goes to prev month, PageDown to next |
| `keyNavigationHome` | Home → today |
| `doubleClickEmitsDayActivated` | `dayActivated` signal spy |
| `outOfMonthDaysDoNotChangeMonth` | Clicking an out-of-month cell still fires `daySelected` with the correct date |

**`tests/ui_smoke/tst_UiSmoke.cpp`** extensions (inserted before `testZeroWarnings`):

| Slot | Covers |
|---|---|
| `testCalendarPageInterviewOnCorrectDay` | Insert interview on 2026-10-15 via repo; open calendar page; navigate to Oct 2026; verify marker on 15th; select 15th; agenda panel has one item with correct title |
| `testCalendarAgendaItemOpensDialog` | From above state: click interview item in agenda → `InterviewDialog` (edit) opens |
| `testCalendarMonthNavigationAndToday` | Click ← twice → Sept 2026; click Today → current month; verify title label |
| `testCalendarDotsMatchAgendaPanel` | Add interview + follow-up on same day; verify 2 dots; select day; verify 2 agenda items |
| `testCalendarLayerToggles` | Uncheck Interviews layer → marker count drops; uncheck Follow-ups → further drop |
| `testCalendarBothThemes` | Light and dark; render calendar with data; zero warnings |
| `testHeaderNewInterviewOpensSingleDialog` | (Part B covers this, leave reference here) |
| `testCalendarCtrlNOpensInterviewDialog` | `QKeySequence::New` on Calendar page → one `InterviewDialog` |

---

## Risks and open questions

| Risk | Mitigation |
|---|---|
| `MonthView` paint complexity | Keep the 42-cell grid unconditional (always 6 rows); do not clip cells — just dim out-of-month day numbers. |
| `AgendaItem.color` removal may break existing tests | Only `tst_InterviewRepository` and `ui_smoke` use `AgendaItem` indirectly; the struct is otherwise untouched in M2 code. Fix any fallout in Part A before the tests are rerun. |
| `ApplicationTableModel` constructor changes (adding `InterviewRepository&`) | `JobsPage`, `ui_smoke::resetJobsData`, and `tst_ApplicationTableModel` all construct the model. Must update all call sites. |
| `CalendarPage` gaining `AppContext&` | `MainWindow` currently creates `CalendarPage` with no arguments; update the one construction site. |
| `MonthView` keyboard navigation on offscreen plugin | The offscreen plugin does not forward native key events the same way. Use `QTest::keyClick` in tests to simulate keypresses. |
| `AgendaService` in `AppContext` | `AgendaService` takes refs to three repos. Order of construction: repos before service (already the case for other services). |
| `FollowUpDialog` (small add-follow-up dialog) | Simple enough (app combo, text, date) to not need a separate file — can be an anonymous `QDialog` inline in `CalendarPage::openAddFollowUp()`, or a tiny `FollowUpDialog.h/.cpp` if it grows. Default: **inline**. |
| Windows cross-platform | `MonthView` uses only `QPainter`, `QDate`, `QColor`, and `QFontMetrics` — all platform-neutral. No `Q_OS_*` code. `InterviewDialog` uses standard Qt widgets. No Windows-specific risk identified. |

---

## Verification and report plan

Report: `docs/reports/M3.md`, AGENTS.md §10 format.

After each commit: `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` passes (zero warnings, all tests green).

### Acceptance criteria checklist

| Criterion (SPEC §12 M3) | Verification method |
|---|---|
| An interview created from the Jobs page appears on the correct day in the calendar | `testCalendarPageInterviewOnCorrectDay` in `ui_smoke` |
| Clicking an agenda item opens it | `testCalendarAgendaItemOpensDialog` in `ui_smoke` |
| Month navigation and "Today" work | `testCalendarMonthNavigationAndToday` in `ui_smoke` |
| The dots on each day match the items in the agenda panel | `testCalendarDotsMatchAgendaPanel` in `ui_smoke` |
| Both themes render with zero warnings | `testCalendarBothThemes` in `ui_smoke`; `testZeroWarnings` gate |
| Interviews tab in `ApplicationDialog` is editable | `testApplicationDialogInterviewsTabEditable` in `ui_smoke` |
| `InterviewDialog` dirty tracking and discard confirmation | `testInterviewDialogCreateAndEdit` + `testInterviewDialogDeleteWithConfirmation` |
| Layer toggles change markers | `testCalendarLayerToggles` |
| Ctrl+N on Calendar page opens `InterviewDialog` | `testCalendarCtrlNOpensInterviewDialog` |
| Header "＋ New → Interview" opens `InterviewDialog` | `testHeaderNewInterviewEnabled` |

Real-launch walk-through (after Part C):
1. `XDG_DATA_HOME=$(mktemp -d) XDG_CONFIG_HOME=$(mktemp -d) ./build/dev-linux/jobprep`
2. Add an application via Jobs page.
3. Add an interview from the context menu → confirm it appears in Calendar.
4. Click the dot → `InterviewDialog` opens with the right data.
5. Navigate month → dots follow; Today returns to the current month.
6. Toggle layers → dots update correctly.
7. Repeat with `QT_QPA_PLATFORM=xcb` for X11.
