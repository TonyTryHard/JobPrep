# M4 Part C: fix-ups round 2 (report accuracy and a few open checks)

Read `AGENTS.md`, `docs/SPEC.md`, `docs/prompts/M4-amendments.md` (v2), `docs/prompts/M4-partC-fixups.md` and `docs/reports/M4-partC.md`.
Round 1 delivered most of the required evidence (hashes, stat, counts, md5sum, clean build). The report still contains statements
that are wrong or not backed by the code. Fix the **report first**, and change code only where a check below shows a real problem.
Local commits only, never push. Code, SPEC and report go in separate commits.

Rules: every claim has a `grep -n` line or a test slot name; every command output is raw and from this session; no claim about SPEC text without quoting it from `docs/SPEC.md`.

## R1. Statements to correct or prove

1. **Section 5 is not SPEC §12.** The ten "criteria" are not in the SPEC (for example "tracks as tabs", "slide-over", "logged study time tally" in the detail panel).
   Run `grep -n -A8 "M4 — Study page" docs/SPEC.md`, copy the real M4 acceptance criteria, and check **those** one by one with a test slot name and one line on what it asserts
   (drag changes status and persists; progress bars follow checklist ticks; a logged session changes the weekly minutes via `totalMinutesInRange`). Delete the invented list.
2. **SessionDialog "auto-increments topic progress if completed hours reach target hours".** There is no target-hours field in the SPEC or in schema 001, and SPEC §4 defines progress.
   Run `grep -nE "progress|target|hours" src/ui/dialogs/SessionDialog.cpp`. If the code does this, remove it (it violates SPEC §4 and amendment A1); if it does not, remove the sentence.
3. **Priority names.** The report says `High > Medium > Low`; the domain enum is low / normal / high. Show the lines in `src/models/TopicTableProxy.cpp` and fix the text.
4. **C5 evidence.** `testMonthViewDoubleClicksDayWithoutOpeningStudyTarget` is not in your own list of 11 added slots and not in the "already existed" list.
   `grep -n` for it. If it does not exist, write it (double-click a day that has a study target -> "New interview" opens, no `studyTopicActivated`), update the slot count and arithmetic.
5. **B7 label.** B7 in the amendments is header routing (`+ New -> Topic` and `+ New -> Study session` call their targets directly, session dialog from any page with the selected topic preselected).
   Ctrl+N is only the last sentence of B7. Give evidence for the header entries (`testHeaderNewSessionAndTopicRouting`, `testNewEntryPointsOpenOneDialog`), then for Ctrl+N including Home and Settings.
6. **Header search (B8) contradicts itself.**
   - The first Part C report said `updateHeader()` and "disabled on Study only"; this report says `onPageChanged()` and "disabled on Study, Calendar, Home and Settings, placeholder 'Search not available on this page'".
   - Commit `5279eb2` did not touch `src/ui/MainWindow.cpp` (only `MainWindow.h`), so both descriptions cannot be true.
   - Run `git diff e6a69b1^..HEAD -- src/ui/MainWindow.cpp | grep -n -i search` and `git show 5279eb2 -- src/ui/MainWindow.h`, and state what header search did on Home / Calendar / Settings at `20e0102` (end of M3).
   - The plan says "no M3 behavior change". If M4 disabled header search on Home/Calendar/Settings, restore the M3 behavior there and keep it disabled only on Study with the tooltip from SPEC §14: `"Use the search box on this page, Ctrl+F"`.
   - Add one slot that walks Study -> Calendar -> Home -> Settings -> Jobs and asserts enabled state, placeholder and tooltip on each page.
7. **Calendar jump scope.** The report says Study targets are activated from the agenda list only; SPEC §14 (1.6) says "agenda or month view". Make the SPEC entry match the code (or the code match the entry).
   Show the test for a topic **deleted in the meantime** (B9) and the one for a topic hidden by the track filter / search, in both board and list view.
8. **List view.** Show the line that sets the **default sort by track**. Name the asserts for Ctrl+F, Enter and Delete in list view (the single slot `testStudyListViewToggleSortAndDelegates` cannot cover everything: say what it covers).
9. **SessionDialog and TrackDialog tests.** Name the assert for: a session logged with "(No topic)" (null `topic_id`); minutes 0 blocked with the inline message;
   duplicate track name in a different case; swatch arrow-key navigation; custom swatch for a non-preset color; delete confirmation text contains the topic count. Add what is missing.
10. **QPA noise explanation.** The Qt file names given (`qtiplabel.cpp`, `qoffscreenwindow.cpp`) look wrong, and `grep "\.raise\(\)"` misses `->raise()` and bare `raise()`.
    Run `grep -rnE "raise\(|activateWindow\(" src/`. Then temporarily disable the filter, run `ui_smoke`, and paste which slot prints "This plugin does not support raise()".
    If only slots that trigger a tooltip emit it, narrow the filter or call `QToolTip::hideText()` after the assertion. Describe it without invented source file names.
11. **Swatch checkmark.** Is the "✓" painted with `QPainter` or is it a text glyph? A text glyph depends on font fallback (Windows vs Linux, no Inter yet) and AGENTS §7 says no emoji as icons. Paint it as a path/line if it is text.
12. **SPEC diff.** Paste `git show 6a8206a -- docs/SPEC.md`. Confirm each entry is there, with its text: no session edit/delete in M4; `TopicTableProxy` (not in plan or SPEC §8); `CalendarPage` takes `TrackRepository&`;
    1440-minute cap; palette is **8** swatches (the older entry said 7); header search wording from item 6. Fix entries that are wrong.
13. **Unreported changes in `5279eb2`:** `src/models/AgendaListModel.cpp`, `src/ui/MainWindow.h`, `resources/styles/theme.qss`. One line each: what and why.
14. **Dialog sizes.** `grep -rnE "setFixedSize|setFixedWidth|setFixedHeight" src/ui`. The 460x400 and 520x440 grabs must come from `sizeHint`/`resize`, not fixed sizes.
15. **Clean tree.** Screenshot PNGs change by a few bytes on every run. After `scripts/screenshots.sh`, paste `git status --short`. Commit regenerated images only if the content really changed, and say so.

## R3. One selection across the whole board (found in the user's manual check)

Screenshot: three cards are highlighted at the same time (one per column: Backlog, In Progress, Review). Each column is its own `QListView` with its own selection model.
There is one detail panel and one set of keyboard actions (Enter, Delete, Alt+Left/Right/Up/Down, "Move to"), so the user cannot tell which card is "the" selected one (SPEC S7: "the selected topic").

Required behavior: exactly one selected topic on the board at any time.
- Selecting a card in one column clears the selection in the other columns. Selection is tracked by topic id, using the same restorer that survives model resets.
- Left/Right between columns moves focus and selects the card in the new column (the old one is cleared). Delete, Enter and the Alt shortcuts act on that one card.
- A card moved by drag and drop, by Alt+Left/Right or by "Move to" stays selected in its new column and only there. `selectTopic(id)` and `openTopic(id)` give the same single selection.
- The detail panel always shows the selected topic; when nothing is selected it follows the existing empty behavior.
- List view is unchanged.

Tests (named slots): click card A in Backlog, then card B in In Progress -> Backlog has no selection, detail panel shows B;
Alt+Right on B -> only Review has a selection; a model reset (checklist tick) keeps exactly one selected card; `selectTopic(id)` for a card in another column clears the old selection.
Add `study-board-selected-{light,dark}.png` to `scripts/screenshots.sh` and to the report. Add a SPEC §14 line only if the final behavior differs from S7.

## R2. Report

Update `docs/reports/M4-partC.md` in place: fix sections 1, 3, 5 and 8 as above, keep the raw outputs that are correct, and add a "Changes in round 2" section with the new hashes from `git log --oneline`.
Do not write "fully complete" or "verified" for anything that only has a screenshot or only a grep. Finish with `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` (raw output).
