# M3 — Calendar & interviews

Status: updated with amendments, approved; executing Part A.
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
| 4a | **ChipBar** | `ChipBar` uses an exclusive `QButtonGroup` — it is **single-select only**, not independently toggleable. The calendar layer chips (Interviews / Follow-ups / Study targets) must each be independently toggleable. The calendar page will use three `QPushButton` (checkable, non-exclusive) styled like `ChipBar` chips with a small dot in kind color. |
| 4b | **`firstDayOfWeek` key** | `SettingsService` already has `firstDayOfWeek()` / `setFirstDayOfWeek(int)` backed by `calendar/firstDayOfWeek` (default 1 = Monday), and emits `firstDayOfWeekChanged(int)`. **Reusable.** |
| 4c | **`PageBase` hooks** | `focusSearch()` and `triggerNew()` exist with default no-op implementations. **Reusable.** |
| 4d | **`StatusStyle` / `Tokens` / `AgendaKindStyle`** | `StatusStyle` provides per-status color and label; `Tokens`/`ColorTokens` provides all surface, border, and accent colors. `AgendaItem` in `Structs.h` currently carries `QColor color`. Per Amendment A3/C5, remove `color` and add `bool past` to `AgendaItem`; keep `QColor` out of `domain`. Colors/icons/labels are provided in UI by `AgendaKindStyle`. |
| 4e | **`InterviewRepository` range query** | `inRange(QDateTime from, QDateTime to)` already exists. Per Amendment A1, bounds are half-open day bounds `[from 00:00:00, to+1 day 00:00:00)` via repository ISO text format query (`start_at >= fromStr AND start_at < toPlusOneStr`). |
| 4f | **`ApplicationRepository` follow-up queries** | Add `byNextActionDateRange(QDate from, QDate to)` query to `ApplicationRepository` with inclusive bounds `[from, to]` on `next_action_date`. Closed-status filter (status NOT IN rejected, withdrawn, ghosted, accepted) is applied in the repository query. |
| 4g | **`TopicRepository` target date queries** | Add `byTargetDateRange(QDate from, QDate to)` query to `TopicRepository` with inclusive bounds `[from, to]` on `target_date`. Topic status filter (`status != 'done'`) is applied in the repository query. |
| 4h | **Jobs "Next step" column refresh on interview change** | Verified: `ApplicationTableModel` constructor ALREADY takes `InterviewRepository&` and ALREADY connects `&m_interviews, &InterviewRepository::changed, this, &ApplicationTableModel::reload`. No constructor or connect changes needed in `ApplicationTableModel`. |
| 4i | **`AgendaItem` struct** | Per Amendments A2 & A3: remove `QColor color` and `#include <QColor>`; add `bool past{false}`. |

---

## Design decisions

| Q | Decision | Default | Reason |
|---|---|---|---|
| Q1 | Which interviews show | All except `Cancelled`; past ones drawn dimmed; outcome shown in subtitle | Cancelled are noise; outcome helps at a glance |
| Q2 | Follow-up semantics | One item per application with `next_action_date` set and status not closed (not rejected/withdrawn/ghosted/accepted); wishlist included. "＋ Follow-up" asks for application and action text, sets selected date as `next_action_date`, confirms before replacing | Simplest interpretation of SPEC C4 |
| Q3 | Study targets | Topics with `target_date` set and `status != done` | M4 brings the Study page; clicking opens a tooltip "Opens in M4" |
| Q4 | `AgendaItem` and color | `AgendaItem.color` removed from domain; `bool past` added; `AgendaKind` maps to style via UI `AgendaKindStyle` | `QColor` must not enter `domain` (Qt Gui dependency) |
| Q5 | Ordering and day assignment | Interview → day of `start_at`; follow-up → day of `next_action_date`; target → day of `target_date`. Order within a day: interviews by time, then follow-ups, then study targets, ties by title | Deterministic, spec-consistent |
| Q6 | Status side effects | Creating an interview does not change the application status automatically | Mentioned as a future option in the dialog (tooltip or comment only) |
| Q7 | Layer toggles persistence | Stored in `SettingsService` under `calendar/layerInterviews`, `calendar/layerFollowUps`, `calendar/layerTargets` (default true). Accessors added to `SettingsService` in Part A | Per-session state would be annoying to lose on restart |
| Q8 | Clicking an agenda item | Interview → `InterviewDialog` (edit mode); follow-up → `ApplicationDialog` (Overview tab); study target → no-op, tooltip "Opens in M4" | SPEC C4, C5, Q3 |
| Q9 | Transactions | `InterviewDialog` only touches `InterviewRepository` → no outer transaction needed for the dialog itself; the "＋ Follow-up" path updates `ApplicationRepository.setNextAction` in one call, so no outer transaction needed there either | Single-repo writes don't need a manually composed transaction |

---

## Part split rationale

Three commits:
- **Part A**: Pre-flight fixes (`tst_Database`), `AgendaItem` domain cleanup, repo queries (`byNextActionDateRange`, `byTargetDateRange`, half-open bounds in `InterviewRepository::inRange`), `SettingsService` layer toggle keys, `AgendaService` with injectable clock and tests in `tests/services/tst_AgendaService.cpp`.
- **Part B**: `InterviewDialog` + editable Interviews tab in `ApplicationDialog` + Jobs page context menu and header menu wiring + `ui_smoke` end-to-end tests.
- **Part C**: `MonthView` (grid only) + `AgendaKindStyle` + `AgendaItemDelegate` + `AgendaListModel` + `CalendarPage` (splitter, toolbar buttons, layer chips, agenda panel) + keyboard nav + `ui_smoke` and unit tests.

| Part | Commit tag | What it delivers |
|---|---|---|
| **A** | `fix(data): pre-flight fixes + AgendaService` | Connection-isolation test, `AgendaItem.color` removal & `past` field, `byNextActionDateRange`, `byTargetDateRange`, half-open range bounds, layer-toggle keys in `SettingsService`, `AgendaService` + full test suite |
| **B** | `feat(ui): InterviewDialog + editable Interviews tab + Jobs wiring` | `InterviewDialog`, editable Interviews tab in `ApplicationDialog`, Jobs context menu "Add interview…" enabled, `+ New → Interview` enabled and routed to `CalendarPage::newInterview`, `+ New → Application` switched to Jobs page, `ui_smoke` test |
| **C** | `feat(ui): MonthView + AgendaListModel + CalendarPage` | `MonthView` (grid-only), `AgendaKindStyle`, `AgendaItemDelegate`, `AgendaListModel`, `CalendarPage` (splitter, real nav widgets, layer chips with kind dot, agenda panel), keyboard nav, today injection, screenshot generation |

---

## Part A — Pre-flight fixes, new repository queries, AgendaService

### Files touched / added

| File | Status | Purpose |
|---|---|---|
| `docs/SPEC.md` | modify | Record in §14 changelog that `AgendaItem` loses `color` and gains `past` |
| `tests/data/tst_Database.cpp` | modify | Add `twoLiveDatabasesAreIndependent` test slot |
| `src/domain/Structs.h` | modify | Remove `QColor color` and `<QColor>`, add `bool past{false}` in `AgendaItem` |
| `src/data/InterviewRepository.cpp` | modify | Update `inRange` to use half-open text comparison `start_at >= fromStr AND start_at < toPlusOneStr` |
| `src/data/ApplicationRepository.h` | modify | Add `byNextActionDateRange(QDate from, QDate to)` declaration |
| `src/data/ApplicationRepository.cpp` | modify | Implement `byNextActionDateRange` (status not closed, date inclusive) |
| `src/data/TopicRepository.h` | modify | Add `byTargetDateRange(QDate from, QDate to)` declaration |
| `src/data/TopicRepository.cpp` | modify | Implement `byTargetDateRange` (status != done, date inclusive) |
| `tests/data/tst_ApplicationRepository.cpp` | modify | Test `byNextActionDateRange` (inclusive bounds, NULL dates, closed statuses) |
| `tests/data/tst_TopicRepository.cpp` | modify | Test `byTargetDateRange` (inclusive bounds, NULL dates, done status excluded) |
| `src/services/SettingsService.h` | modify | Add `calendarLayerInterviews`, `calendarLayerFollowUps`, `calendarLayerTargets` getters/setters and change signals |
| `src/services/SettingsService.cpp` | modify | Implement layer settings (defaults true) |
| `src/services/AgendaService.h` | **new** | `AgendaService` class with injectable `Clock` (`std::function<QDateTime()>`) |
| `src/services/AgendaService.cpp` | **new** | Implementation of `items(QDate from, QDate to)` |
| `src/app/AppContext.h` | modify | Add `AgendaService& agendaService()` accessor |
| `src/app/AppContext.cpp` | modify | Instantiate `AgendaService` |
| `tests/services/CMakeLists.txt` | modify | Add `tst_AgendaService` test executable |
| `tests/services/tst_AgendaService.cpp` | **new** | Full test suite for `AgendaService` |
| `CMakeLists.txt` | modify | Add `AgendaService.h/.cpp` to `jobprep_core` |

### `AgendaService` design (Amendment A1, A2)

```cpp
namespace JobPrep::Services {

class AgendaService {
public:
    using Clock = std::function<QDateTime()>;

    explicit AgendaService(
        Data::InterviewRepository& interviews,
        Data::ApplicationRepository& apps,
        Data::TopicRepository& topics,
        Clock clock = [] { return QDateTime::currentDateTime(); });

    void setClock(Clock clock);

    /// Returns items whose date falls in [from, to] inclusive, sorted by
    /// (date, kind, time, title). kind order: Interview < FollowUp < StudyTarget.
    QList<Domain::AgendaItem> items(QDate from, QDate to) const;
};
}
```

- **Bounds and Queries**:
  - `InterviewRepository::inRange` called with `from` 00:00:00 and `to.addDays(1)` 00:00:00. SQL: `WHERE start_at >= ? AND start_at < ?`. Exclude `Cancelled` interviews.
  - `ApplicationRepository::byNextActionDateRange(from, to)`: SQL: `WHERE next_action_date >= ? AND next_action_date <= ? AND status NOT IN ('rejected', 'withdrawn', 'ghosted', 'accepted')`. Wishlist is included.
  - `TopicRepository::byTargetDateRange(from, to)`: SQL: `WHERE target_date >= ? AND target_date <= ? AND status != 'done'`.
- **Item mapping**:
  - Interview: `date = startAt.date()`, `time = startAt.time()`, `title = EnumStrings::toString(type) + " · " + company`, `subtitle = place + " · " + interviewer`, `refId = id`, `past = (startAt < now)`.
  - Follow-up: `date = *nextActionDate`, `time = nullopt`, `title = "Follow-up · " + company`, `subtitle = nextAction`, `refId = appId`, `past = (date < now.date())`.
  - Study target: `date = *targetDate`, `time = nullopt`, `title = "Target · " + title`, `subtitle = trackName`, `refId = topicId`, `past = (date < now.date())`.
- **Deterministic ordering**:
  - Primary: `item.date` ascending.
  - Secondary: `item.kind` ascending (`Interview` < `FollowUp` < `StudyTarget`).
  - Tertiary: `item.time` (for interviews; items with time sort before items without time, earliest time first).
  - Quaternary: `item.title` ascending.

### Part A test table

| Test target | Slot | What it covers |
|---|---|---|
| `tst_Database` | `twoLiveDatabasesAreIndependent` | Two `TestDatabase` instances concurrently open `:memory:`; destroy first; second remains functional (Amendment 2) |
| `tst_ApplicationRepository` | `byNextActionDateRangeQueries` | Inclusive bounds, null dates ignored, active & wishlist included, closed statuses excluded (Amendment A1) |
| `tst_TopicRepository` | `byTargetDateRangeQueries` | Inclusive bounds, null dates ignored, open topics included, done topics excluded (Amendment A1) |
| `tst_AgendaService` | `noItemsForEmptyDatabase` | Empty result when DB has no data |
| `tst_AgendaService` | `interviewAppearsOnItsStartDay` | One interview → exactly one item on start date |
| `tst_AgendaService` | `cancelledInterviewExcluded` | `outcome == Cancelled` → not returned |
| `tst_AgendaService` | `pastInterviewFlagged` | Past interview (`startAt < now`) has `past == true`; future has `past == false` (Amendment A2) |
| `tst_AgendaService` | `dayBoundaryInterviewCheck` | Interview at 00:00:00 on `from`, 23:59:59 on `to` are included; 00:00:00 on `to+1` is excluded (Amendment A1) |
| `tst_AgendaService` | `followUpForActiveAndWishlistApplications` | Active & wishlist applications appear; closed applications excluded |
| `tst_AgendaService` | `pastFollowUpFlagged` | `next_action_date < now.date()` has `past == true` |
| `tst_AgendaService` | `studyTargetForOpenTopics` | Backlog/in progress/review topics appear; done topics excluded |
| `tst_AgendaService` | `rangeAcrossMonthBorder` | 31 Oct to 1 Nov: items on both days returned |
| `tst_AgendaService` | `rangeAcrossYearEnd` | 31 Dec to 1 Jan: items on both days returned |
| `tst_AgendaService` | `leapDayMonth` | Range covering 29 Feb 2028: item on 29 Feb returned |
| `tst_AgendaService` | `lateNightInterview` | 23:30 start on date D falls on date D |
| `tst_AgendaService` | `deterministicOrdering` | Correct sorting: date, kind (interview, follow-up, target), time, title (Amendment A5) |
| `tst_AgendaService` | `singleDayRange` | `from == to` returns only items on that single day |

---

## Part B — InterviewDialog, editable Interviews tab, Jobs wiring

### Files touched / added

| File | Status | Purpose |
|---|---|---|
| `src/ui/dialogs/InterviewDialog.h` | **new** | Dialog to create/edit one interview (SPEC §3.4) |
| `src/ui/dialogs/InterviewDialog.cpp` | **new** | Implementation (AUTOMOC) |
| `src/ui/dialogs/ApplicationDialog.h` | modify | Make Interviews tab editable; add interview add/edit/delete actions |
| `src/ui/dialogs/ApplicationDialog.cpp` | modify | Interviews tab: table + Add/Edit/Delete buttons; hint note on immediate save |
| `src/ui/pages/JobsPage.cpp` | modify | Enable "Add interview…" context menu action |
| `src/ui/pages/JobsPage.h` | modify | Add `openAddInterview(int appId)` private slot |
| `src/ui/MainWindow.cpp` | modify | Route header `+ New → Application` to Jobs page; route `+ New → Interview` to `CalendarPage::newInterview`; shortcut Ctrl+N uses `triggerNew` (Amendment B1) |
| `src/ui/pages/CalendarPage.h` | modify | Constructor takes explicit repo/service refs (Amendment B2); add `newInterview(QDate)` and `triggerNew()` |
| `src/ui/pages/CalendarPage.cpp` | modify | Implement `newInterview(QDate)` and `triggerNew()` opening `InterviewDialog` |
| `CMakeLists.txt` | modify | Add `InterviewDialog.h/.cpp` to `jobprep_core` |
| `tests/ui_smoke/tst_UiSmoke.cpp` | modify | Add Part B acceptance test slots (Amendment B1, B4) |

### Part B Details & Amendments
- **Header routing (Amendment B1)**:
  - Header `+ New → Application` switches to Jobs page and calls `m_jobsPage->newApplication()`.
  - Header `+ New → Interview` calls `m_calendarPage->newInterview(selectedOrTodayDate)`.
  - Ctrl+N invokes `triggerCurrentPageNew()` calling `page->triggerNew()`.
- **Explicit Injection (Amendment B2)**:
  - `CalendarPage` constructor receives explicit references:
    `CalendarPage(Data::InterviewRepository&, Data::ApplicationRepository&, Data::TopicRepository&, Services::AgendaService&, Services::SettingsService&, Theme::ThemeManager&, QWidget* parent = nullptr)`.
- **Dialog behavior (Amendment B3)**:
  - `InterviewDialog` application combo lists non-closed applications by default; in Edit mode the interview's application is always included even if closed.
  - Interviews tab in `ApplicationDialog` states in hint: interview changes are saved immediately to the database and are not part of Overview "Discard changes?".
- **Part B tests in `ui_smoke`**:
  - `testInterviewDialogCreateAndEdit`: full form fill, edit, dirty tracking, discard confirmation.
  - `testInterviewDialogValidation`: app required, duration >= 1.
  - `testInterviewDialogDeleteWithConfirmation`: delete button in edit mode with confirmation.
  - `testApplicationDialogInterviewsTabEditable`: tab editable when saved; hint when new.
  - `testHeaderNewApplicationOnCalendarPage`: on Calendar page, header `+ New → Application` switches to Jobs and opens one `ApplicationDialog` (Amendment B1).
  - `testJobsContextMenuAddInterviewEndToEnd`: Jobs row context menu → "Add interview…" → save with date → Calendar reflects marker (Amendment B4).

---

## Part C — MonthView, AgendaListModel, CalendarPage

### Files touched / added

| File | Status | Purpose |
|---|---|---|
| `src/ui/theme/AgendaKindStyle.h` | **new** | Style helper (color token, label, icon) per `AgendaKind` (Amendment C5) |
| `src/ui/theme/AgendaKindStyle.cpp` | **new** | Implementation of `AgendaKindStyle` |
| `src/ui/delegates/AgendaItemDelegate.h` | **new** | Custom delegate for items in the agenda panel (Amendment C5) |
| `src/ui/delegates/AgendaItemDelegate.cpp` | **new** | Implementation of `AgendaItemDelegate` |
| `src/ui/widgets/MonthView.h` | **new** | Custom-painted month grid widget (grid only) (Amendment C1, C4, C9) |
| `src/ui/widgets/MonthView.cpp` | **new** | Implementation of `MonthView` |
| `src/models/AgendaListModel.h` | **new** | `QAbstractListModel` exposing `KindRole`, date, time, title, subtitle (Amendment C5) |
| `src/models/AgendaListModel.cpp` | **new** | Implementation of `AgendaListModel` |
| `src/ui/pages/CalendarPage.h` | modify | Full calendar page with splitter, real nav widgets, layer chips (Amendment C1, C7, C8) |
| `src/ui/pages/CalendarPage.cpp` | modify | Full implementation of calendar page |
| `resources/resources.qrc` | modify | Register any new SVG icons needed for agenda kinds |
| `tests/models/CMakeLists.txt` | modify | Add `tst_AgendaListModel` |
| `tests/models/tst_AgendaListModel.cpp` | **new** | Tests for `AgendaListModel` and `AgendaKindStyle` |
| `tests/ui_smoke/CMakeLists.txt` | modify | Add `tst_MonthView` |
| `tests/ui_smoke/tst_MonthView.cpp` | **new** | Tests for `MonthView` grid math, selection, keyboard nav |
| `tests/ui_smoke/tst_UiSmoke.cpp` | modify | Add Calendar smoke tests & screenshot captures |
| `CMakeLists.txt` | modify | Add Part C sources |

### Part C Details & Amendments
- **Grid only (Amendment C1)**: Prev/Next/Today buttons and month title are real widgets in `CalendarPage` layout (e.g. `QToolButton`s with icons and tooltips), not painted in `MonthView`.
- **Selection rule (Amendment C2)**: Clicking or navigating to an out-of-month day updates displayed month to match and emits `monthChanged`.
- **Marker Range (Amendment C3)**: Markers loaded for entire 42-cell visible range `[MonthView::visibleRange().first, MonthView::visibleRange().second]`.
- **Marker API (Amendment C4)**:
  ```cpp
  struct DayMarkers {
      QList<Domain::AgendaKind> kinds; // up to 3
      int total{0};
  };
  ```
  Pass kinds, not `QColor`.
- **Layering (Amendment C5)**: No `QColor` or `Tokens` in `AgendaListModel`. `AgendaKindStyle` in `ui/theme/` maps `AgendaKind` to color token, display label, and icon.
- **Double-click day & shortcuts (Amendment C6)**:
  - Double-click a day in `MonthView` (`dayActivated`) opens `InterviewDialog(Create, date = selectedDay)`.
  - "+ Interview" button and Ctrl+N on CalendarPage use `selectedDay()`.
- **Layout (Amendment C7)**: `QSplitter` between MonthView and Agenda panel; save/restore state via `SettingsService`.
- **Layer chips (Amendment C8)**: 3 non-exclusive `QPushButton`s styled like `ChipBar` with kind dot.
- **Injectable today (Amendment C9)**: `MonthView` and `CalendarPage` take `std::function<QDate()>` for today. Grid tests for 2023-09-25 Mon-start vs 2023-10-01 Sun-start, Feb 2028, etc.
- **Report & Screenshots (Amendment C10)**: Grab screenshots of Calendar page in light and dark mode, save to `docs/reports/img/`, list unverified items.

---

## Verification and report plan

Report: `docs/reports/M3.md`, AGENTS.md §10 format.
Screenshots: `docs/reports/img/calendar-light.png`, `docs/reports/img/calendar-dark.png`.

After each part: `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` passes (zero warnings, all tests green).
