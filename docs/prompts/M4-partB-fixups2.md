# M4 Part B fix-ups, round 2

Read `docs/prompts/M4-amendments.md` (v2), `docs/prompts/M4-partB-go.md`, `docs/prompts/M4-partB-fixups.md` and `docs/reports/M4-partB-fixups.md`.
The round-1 report has numbers that do not add up and some claims that do not match the code or the prompts. Fix the items below.
Local commits only, never push. Prompt files go in their own commit. Write the answers to `docs/reports/M4-partB-fixups2.md`.
**Paste raw terminal output; do not retype or summarize command output.**

## 1. Test counts and invented output

The user ran the commands on the real binaries. Facts:
- `tst_MonthView -functions` lists **13** slots: `gridStartsOnMondayFirstAndSundayFirst`, `february2028Grid`, `monthBeginningOnFirstWeekday`,
  `todayFromProviderAndInitialSelection`, `clickingOutOfMonthDayFollowsMonth`, `keyboardNavigation`, `homeEndSelectWeekBounds`, `tabFocusEntersAndLeaves`,
  `pageUpDownMoveMonth`, `activateKeysEmitDayActivated`, `doubleClickEmitsDayActivated`, `markersOnTrailingDimmedDay`, `dayRectNullOutsideGrid`.
- `ui_smoke -functions | wc -l` prints **47**.

The round-1 report said `tst_MonthView` has 11 slots and printed a list of names (`testMultiDaySpansDrawn`, `testCustomColorsRespected`, ...) that do not exist,
and said `ui_smoke` has 49 test slots. That "command output" was not real output. Explain in the report how it was produced, and correct it.
From now on every command output in a report must be copied from a real run of that command in this session.

Run, adjusting the path to the real binaries, and paste the raw output:

```bash
QT_QPA_PLATFORM=offscreen ./build/dev-linux/tests/ui_smoke/ui_smoke 2>&1 | grep -E "^(PASS|FAIL|SKIP)|Totals"
git log --oneline -8
git show --stat HEAD
```

Then replace the file-and-line column of the amendment checklist by the raw output of `grep -n` commands that find each cited line
(round-1 numbers cannot be trusted until they are shown this way).

## 2. Priority marker

SPEC priorities are Low / Normal / High. The report says the card shows "P0, P1". Use the labels from `StatusStyle` (Low/Normal/High), readable without colour,
with a tooltip. Show the delegate lines and regenerate the screenshots.

## 3. Page API must match the go file

The go file says `StudyPage::newTopic()` has no arguments: it focuses the Backlog quick-add (and does nothing visible when there are no tracks).
The report has `newTopic(title, trackId)` that creates a topic. Part C calls `newTopic()` from the header, so restore the no-argument version.
If a creating helper is still useful, give it another name (for example `createTopic(...)`). Fix the SPEC §14 entry and `testStudyPageApi`.

## 4. Wrong claims in the round-1 report

1. **"(B3)" for `ChipBar`.** B3 in `M4-amendments.md` is "Cards: due chip red only with OverdueRole, priority marker readable without colour".
   No amendment asked for a `ChipBar` change. State the real reason for it and what exactly changed. No fake amendment reference.
2. **Amendment checklist.** Item "B6" in the report lists the checklist; that content belongs to B5. B6 is the toolbar: chips, the disabled "Manage tracks...",
   the `[+ Topic]` button calling `newTopic()`. Give B6 evidence (file and line) and fix the labels.
3. **"Matching all specifications."** Remove it. Give a short, honest plan-vs-result with the known gaps.

## 5. Missing or partial tests (add them)

- Context menu "Move to > status" changes the status and persists.
- After a checklist tick, **scroll position and detail-panel focus** survive as well as the selection (A5).
- Real page-level selection restore after **insert, delete and reorder** (not only the helper test).
- Checklist **edit** and **Alt+Up/Down reorder** (and drag reorder if it can be driven offscreen; otherwise say so).
- Study splitter state is stored and restored (like `testCalendarSplitterStatePersisted`).

## 6. Splitter saving

The report says the splitter state is saved "on destruction / state save". Save it on `splitterMoved` (or equivalent) so a crash does not lose it.
Confirm and show the lines.

## 7. Drag feedback (found in the user's manual check)

With the mouse, a card dragged over a column shows **no marker** where it will land (no insertion line or highlight). Fix it:
- The view must show where the drop will go: an accent-coloured insertion line between cards, drawn by a small `TopicListView : QListView` subclass
  (the default drop indicator is probably hidden by the QSS or the delegate), and a visible highlight of the whole column while a drag is over it.
  The highlight is also the only feedback for an **empty column**.
- Check `setDragEnabled(true)`, `setAcceptDrops(true)`, `setDropIndicatorShown(true)`, `setDefaultDropAction(Qt::MoveAction)` and that the dragged card
  has a visible drag pixmap.
- Colours come from the theme tokens (both themes), no fixed pixel constants.
- Test: send a synthesized `QDragMoveEvent` to the view and assert that the indicator state/rectangle is set (expose it for tests), and cleared after drag leave or drop.
  Add it to the screenshots if it can be grabbed during a synthesized drag move.

## 8. Also give

Hashes of every fix-up commit, the full final `check.sh` summary lines, and an updated "Not verified by agent, user to check" list.
Zero warnings, `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green.
