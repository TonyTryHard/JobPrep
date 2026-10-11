# M3 Part C — Calendar month grid, agenda panel and layer chips

Covers Part C of Milestone M3 (plan `docs/plans/M3-plan.md`, amendments
`docs/prompts/M3-amendments.md`, go-file `docs/prompts/M3-partC-go.md`), delivered in two
local commits:

- `a6303a6` — **C0**: Part B carry-overs (Interviews-tab tooltips/mnemonics, `InterviewDialog`
  keyboard short-cuts, header routing test, inclusive `inRange` boundary test, SPEC §14 entries,
  dead-seam removal). The go-file itself was committed with this commit.
- `f6c5433` — **Part C**: `MonthView`, `AgendaListModel`, `AgendaKindStyle`,
  `AgendaItemDelegate`, `FollowUpDialog`, the full `CalendarPage`, settings, tests and screenshots.

## Plan versus result

| Plan item (file table) | Result |
|---|---|
| `src/ui/theme/AgendaKindStyle.{h,cpp}` | Added: `color/label/iconName/icon` per `AgendaKind` |
| `src/ui/delegates/AgendaItemDelegate.{h,cpp}` | Added: 44 px row, kind dot icon, subtitle, past-interview dimming |
| `src/ui/widgets/MonthView.{h,cpp}` | Added: grid-only six-week painter, `DayMarkers`, `visibleRange()`, keyboard nav |
| `src/models/AgendaListModel.{h,cpp}` | Added: neutral list, `KindRole/IdRole/SubtitleRole/TimeRole/DateRole/PastRole` |
| `src/ui/pages/CalendarPage.{h,cpp}` | Rewritten: toolbar with real widgets, layer chips, persistent `QSplitter`, agenda panel |
| `resources/resources.qrc` + kind SVGs | Added `agenda-interview/-followup/-target.svg` |
| `tests/models/tst_AgendaListModel.cpp` | Added (5 slots, includes an `AgendaKindStyle` test replacing `colorRoleMapsByKind`) |
| `tests/ui_smoke/tst_MonthView.cpp` | Added (11 slots: grid math, selection rule, keyboard, markers) |
| `tests/ui_smoke/tst_UiSmoke.cpp` | Added 8 Calendar slots + screenshot capture |
| `CMakeLists.txt`, test `CMakeLists.txt` | Updated with the new sources/targets |
| **Not in the plan** | `src/ui/dialogs/FollowUpDialog.{h,cpp}`, `src/services/SettingsService.{h,cpp}` (layer toggles + splitter state), `resources/styles/theme.qss` — see Deviations |

Everything in the plan is implemented as written; the three additions above are the only
scope beyond the plan's file table.

## What changed

- **`MonthView`** (grid only, C1): custom `QPainter` month grid, six full weeks including
  leading/trailing neighbour days; `visibleRange()` returns all 42 cells (C3); `DayMarkers`
  carries `kinds` (max 3) and `total` for "+N" (C4); selection that lands outside the shown
  month follows the month and emits `monthChanged` (C2); arrows ±1/±7, PageUp/PageDown,
  Enter/Space = activate; injectable today provider (C9).
- **`AgendaListModel`**: flat list, neutral roles only — no `QColor`, `Tokens` or `ui/`
  include (C5).
- **`AgendaKindStyle`** (`ui/theme`): kind → color/label/icon; the single source of kind
  styling for the grid markers, the agenda delegate and the layer chips.
- **`AgendaItemDelegate`**: kind dot icon, title, muted subtitle, right-aligned time; dims
  only past interviews; study targets get a "opens in M4" tooltip.
- **`FollowUpDialog`**: picks a non-closed application + action text and writes
  `next_action_date` for the selected day; asks before replacing an existing follow-up.
- **`CalendarPage`**: prev/next/Today `QToolButton`s + month title, three non-exclusive
  layer chips, `QSplitter` (month | agenda) with persisted sizes, agenda panel with empty
  state and `+ Interview` / `+ Follow-up` / `+ Study target` (disabled, tooltip) actions;
  double-click a day opens a new interview for that day (C6); Ctrl+N uses the selected day.
- **`SettingsService`**: `calendarLayerInterviews/FollowUps/Targets` with `*Changed`
  signals and `calendarSplitterState()`/`setCalendarSplitterState()`; `firstDayOfWeekChanged`
  drives the grid (C7/C8).

## Amendment checklist

- **C1 — grid only.** Prev/Next/Today and the month title are real widgets in `CalendarPage`;
  `MonthView` paints only the grid.
- **C2 — selection rule.** Out-of-month selection (click, arrows at edge, PageUp/Down) moves
  the shown month and emits `monthChanged`; `tst_MonthView` covers click and PageUp/Down.
- **C3 — marker range.** Markers/items load for `visibleRange()`; `markersOnTrailingDimmedDay`
  asserts a marker on a trailing day.
- **C4 — marker API.** `DayMarkers{kinds ≤3, total}`; `MonthView` paints "+N" via a public
  `markersFor()`.
- **C5 — layering.** No color/Tokens in `models/` (grep below); `AgendaKindStyle` added and
  used by markers/delegate; the old `colorRoleMapsByKind` test is replaced by `agendaKindStyle`.
- **C6 — missing scope.** Double-click stays; `newInterview(QDate)`, the panel "+ Interview"
  and Ctrl+N use the selected day. Smoke: `testCalendarNewInterviewFromDay`.
- **C7 — layout.** `QSplitter` with persisted state; smoke `testCalendarSplitterStatePersisted`.
- **C8 — layer chips.** Checkable chips using the shared chip QSS with a kind-colored dot;
  non-exclusive; visibility persisted. Smoke `testCalendarLayerChipsFilterAgenda`.
- **C9 — injectable today.** `MonthView`/`CalendarPage` take `std::function<QDate()>`; grid
  start/end tests for Oct 2023 (Mon/Sun), Feb 2028 and a month starting on the first weekday.
- **C10 — report.** This file plus light/dark screenshots under `docs/reports/img/`.

## How to run

```bash
cd /home/tony/projects/JobPrep
CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
```

## How verified

Environment: Linux 26.04, GCC 15.2.0, Ninja, `dev-linux` preset, `CMAKE_BUILD_TYPE=Debug`,
`JOBPREP_WERROR=ON`, Qt 6.10.2.

- `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` — configure OK, build OK with zero
  warnings, `100% tests passed, 0 tests failed out of 16` (14 → 16 with `tst_MonthView` and
  `tst_AgendaListModel`).
- `tst_MonthView` — 11 slots pass (grid start/end, today, selection-follows-month, arrows,
  PageUp/Down, Enter/Space, double-click, trailing-day markers, null rect).
- `tst_AgendaListModel` — 5 slots pass (empty model, roles, past flag, `AgendaKindStyle`
  distinct colors/labels/icons, no color roles exposed).
- `ui_smoke` — 34 functional slots pass under `QT_QPA_PLATFORM=offscreen` (8 new Calendar
  slots: markers/agenda, layer-chip filtering, double-click/triggerNew, follow-up end-to-end,
  overwrite prompt, splitter persistence, themes, screenshots).
- Layering grep (`C0` item 2):
  `grep -rn "StatusStyle\|ui/theme\|ui/Tokens\|QColor" src/models/` → only a comment at
  `src/models/ApplicationTableModel.cpp:105`; no code or include match.
- Screenshots generated by the offscreen smoke run via `QWidget::grab()`
  (`JOBPREP_REPORT_IMG_DIR`): `calendar-empty`, `calendar-markers`, `calendar-selected`, each
  in light/dark at 100% and a 1.5× scaled render, saved in `docs/reports/img/`.

## Deviations and assumptions

- **Beyond the plan's file table:** `FollowUpDialog` (needed for the SPEC §3.4 C4 follow-up
  entry point), `SettingsService` layer toggles + splitter state, and the `theme.qss`
  calendar/layer-chip styles. Recorded in SPEC §14 (1.5).
- **Layer chips use a kind dot, not the kind SVG.** Amendment C5 assigns the per-kind icon to
  the chips while C8 asks for "a small dot in the kind color"; the chips follow C8 (dot) and
  the agenda rows use `AgendaKindStyle::icon` (C5). SPEC §14 1.5 notes this.
- **`CalendarPage` constructor change:** a trailing defaulted
  `std::function<QDate()> todayProvider` was added (C9); the B2 reference params are unchanged.
  SPEC §14 1.5.
- **Screenshot "150%".** The offscreen platform is fixed at 100% DPR, so the 1.5× images are a
  smooth upscale of a 1× grab, not a true 1.5× device-pixel render (noted below).
- `AgendaItem::past` remains a neutral fact; only past interviews are dimmed (follow-ups and
  targets keep full contrast).

## Not verified by agent, user to check

- True 150% device-pixel-ratio rendering of the calendar (only a 1.5× raster upscale was
  produced offscreen).
- Contrast of the kind dots / markers in the dark theme by eye (no image viewer available to
  the agent).
- Live Wayland/X11 manual click-through of the new Calendar page (no GUI automation tool;
  only the offscreen smoke run and the test binary were exercised). Qt's own
  `QWaylandTextInputv3` messages appear when running the test binary on native Wayland; the
  zero-warning gate correctly runs offscreen.

## Next step

M3 is complete; proceed to M4 (Study tracks/topics board and sessions) per `docs/SPEC.md`,
starting with a short plan for approval.
