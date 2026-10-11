# M3 fix-ups

Answers to `docs/prompts/M3-fixups.md`, one section per item. All fixes are in the local
commit after this file; `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` is green (16/16,
zero warnings).

## 1. Create mode (`ApplicationDialog` Interviews tab, C0 item 1)

It was already the **disabled + tooltip** case, not save-first. `updateInterviewButtons()`
(`src/ui/dialogs/ApplicationDialog.cpp:426`) sets `m_addInterviewBtn->setEnabled(hasRow)` where
`hasRow = m_app.id != 0`, and the tooltip is
"Save the application first to add interviews" when unsaved. A second path
(`ApplicationDialog.cpp:378`) uses "Save the application first to add interviews." for the hint
label.

The smoke test `testInterviewTabDisabledUntilSaved` already checked disabled + non-empty
tooltip; it now asserts the tooltip contains "Save" so the reason is verified, not just that
*some* tooltip exists. No production change was needed.

## 2. Stored interview time (C0 item 5)

Interview `start_at` is stored through `SqlStatement::bind(int, const QDateTime&)` →
`DbFormat::toText(QDateTime)` = `yyyy-MM-ddTHH:mm:ss` (`src/data/DbFormat.cpp:17`). That is
**whole-second, no fractional seconds, no offset**; `DbFormat::now()` also zeroes the
milliseconds. Reads go through `dateTimeFromText()` with the same format.

Because the column is exact fixed-width text, the inclusive bound
`start_at >= ? AND start_at <= ?` with `to = 23:59:59` is correct and matches the whole-second
storage; no value can fall between `23:59:59` and the next day. The half-open form from the
amendments was **not** needed, and no change was made.

## 3. Settings (Part A vs Part C)

- **Already there** (pre-flight commit `84785fe`, before Part A): `firstDayOfWeek()`/setter/
  `firstDayOfWeekChanged` (key `calendar/firstDayOfWeek`) and
  `calendarLayerInterviews/FollowUps/Targets()` with `*Changed` signals (keys
  `calendar/layerInterviews`, `calendar/layerFollowUps`, `calendar/layerTargets`). So the Part A
  report was right that the toggles pre-existed; the Part C report's "What changed" bullet was
  inaccurate and has been corrected.
- **New in Part C** (`f6c5433`): `calendarSplitterState()`/`setCalendarSplitterState()`
  (key `calendar/splitterState`) — the only settings API Part C added.

There is **one definition of each key and each accessor** (grep of
`SettingsService.cpp` shows a single occurrence per key); no duplicate key and no duplicate
API was introduced.

## 4. Test counts

Real numbers, from `git show a6303a6:tests/ui_smoke/tst_UiSmoke.cpp` and the Part C diff:

- Before Part C (commit `a6303a6`): **26** functional `test*` slots (counting
  `testZeroWarnings`; `initTestCase`/`cleanupTestCase` excluded).
- Added by Part C: **8** slots —
  `testCalendarPageMarkersAndAgenda`, `testCalendarLayerChipsFilterAgenda`,
  `testCalendarNewInterviewFromDay`, `testCalendarFollowUpEndToEnd`,
  `testCalendarFollowUpOverwritePrompt`, `testCalendarSplitterStatePersisted`,
  `testCalendarThemes`, `testCalendarScreenshots`.
- Total now: **34** functional slots (`+ initTestCase + cleanupTestCase` = 36 QTest functions,
  which is why `ui_smoke` reports "36 passed").

The prompt's "25 before + 8 = 33" was off by one; the real base was 26, so 26 + 8 = 34. The
Part C report listed the 8 slot names and the correct base in this fix-up.

## 5. Layering

```
$ grep -rnE "QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models
(no output)
```

No match in any of the four lower layers — only `src/ui` uses those types.
`AgendaListModel` exposes no `Qt::DecorationRole` and includes no `QIcon` (its headers are
`QAbstractListModel`, `QList`, `domain/Structs.h`); a follow-up
`grep -rnE "DecorationRole|QIcon|Qt::Decoration" src/models` also returns nothing. Icons live
in `AgendaItemDelegate`/`AgendaKindStyle` (`src/ui`). Nothing to move.

## 6. Keyboard

- **Tab / Shift+Tab focus:** yes. `MonthView` sets `setFocusPolicy(Qt::StrongFocus)`
  (`src/ui/widgets/MonthView.cpp:39`), and `StrongFocus` includes `TabFocus`, so it joins the
  tab chain. New test `tabFocusEntersAndLeaves` builds a small host with a `QLineEdit` before
  and after the view and asserts Tab enters and Shift+Tab leaves it (and that the focus policy
  has the `TabFocus` flag).
- **Visible focus cell:** yes. `focusInEvent`/`focusOutEvent` track `m_hasFocus` and repaint;
  the selected cell is filled with the accent colour and the text switches to `accentText`
  only while focused (`MonthView.cpp:265`, `:280`).
- **Home/End:** this was **missing**. Added `startOfWeek()` (respects `firstDayOfWeek`) and
  `Key_Home`/`Key_End` handling so Home selects the first day of the week and End the last
  (`MonthView.cpp`). New test `homeEndSelectWeekBounds` covers both Monday-first and
  Sunday-first. Documented in SPEC §14.

## 7. Screenshots

Deleted all six `*@1.5x.png` files from `docs/reports/img/` and removed the upscale code from
`testCalendarScreenshots` so they are not recreated. Kept the six 100% images
(`calendar-{empty,markers,selected}-{light,dark}.png`). Removed the 1.5× mentions from the
Part C report (`M3-partC.md`).

## 8. Go-file commit hygiene

The Part C go-file `docs/prompts/M3-partC-go.md` was committed inside the code commit
`a6303a6` (and `M3-partB-go.md` inside `1d12877`). Per instruction there is **no history
rewrite**; this is just recorded here. From now on prompt files are kept out of code commits —
this fix-up's code/docs land in one commit and `docs/prompts/M3-fixups.md` is added in a
separate commit.

## Verification

```
$ CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh
100% tests passed, 0 tests failed out of 16
==> Quality check PASSED with zero warnings and all tests green!
```

`tst_MonthView` now runs 13 slots (11 + `homeEndSelectWeekBounds` + `tabFocusEntersAndLeaves`),
`ui_smoke` 34 functional slots; all green. Behaviour change (Home/End + focus) recorded in
SPEC §14.
