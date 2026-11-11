# M7 Part B fix-up: evidence and test-quality corrections

Read `docs/reports/M7-partB.md`, `docs/prompts/M7-partB-go.md` (its rules and amendments 1a to 2g still apply), `AGENTS.md`.
Local commits only, never push, never tag. Never run the app binary without an isolated `HOME`/`XDG_*`. Raw output with the exact command above it, nothing trimmed (no `...`), `(no output)` for empty, hashes only from `git log --oneline`, totals by command, no claim without a command. Scratch harnesses and throw-away worktrees stay outside the repository and are removed afterwards (`git worktree prune`, show `git worktree list` raw at the end).
Code commits only where a task below says so; then docs/report commit last. Do **not** start Part C.

## Why this fix-up exists

The Part B report has evidence defects, so its claims are not proven yet:
1. §3.3 and §4.5 show the command `... -functions` above output that contains `PASS`/`FAIL` lines. `-functions` only lists test names; it cannot print those lines. The output was not produced by the command above it.
2. §3.3 labels the failing-before result "simulated via ink bounding box collision". The numbers in that `FAIL!` line (`titleInk=[14,32] subInk=[21,38]`) are the ones from the scratch harness. A real run of the new test on the old code was not shown.
3. §4.5 "failing-before" shows only **one** row (`InterviewDateEdit`) failing on the unpatched class; on unpatched code all five rows must fail, and `TopicDetailPanelDateEdit` runs first.
4. The Item 1 test has two names: `testDelegateAndWidgetTextOverlap` (§3.3) and `testTodayDelegateTwoLineLayout` (§8).
5. The old-code sibling-overlap hits (amendment 1d: run the check on the **current/old** code first and paste every hit) are not shown; the report only says "zero collisions" for the new code.
6. B0 items 8 and 9 are summarised without their raw numbers.

## F1. Real failing-before runs (no simulation)

Create a throw-away worktree of the parent of Part B (`git worktree add <dir outside the repo> 0f4b9b2`), copy **only the new test files** (`tests/ui/tst_HiDpi.cpp`, `tests/ui/tst_Shortcuts.cpp` and any helper they need) into it, build just `tst_HiDpi` and `tst_Shortcuts` there, and run:

```
QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=1.5 <worktree>/build/.../tst_HiDpi <the Item 1 slot name>
QT_QPA_PLATFORM=offscreen <worktree>/build/.../tst_Shortcuts testDateControlsKeyboardPopup
```

Paste the complete raw output of both runs (all five data rows must fail on the old code; if one row passes on old code, explain why, it means the row tests nothing). Then the same two commands on the new code (`build/dev-linux`), complete raw output. If a test does not compile on the old tree because it needs new API (for example `computeLayout`), say exactly which symbol and instead run the **old delegate** through a copy of the test that paints it (title only and subtitle only into transparent `QImage`s) so that the old failure is real.

## F2. One name, correct commands

`grep -n "testDelegateAndWidgetTextOverlap\|testTodayDelegateTwoLineLayout" tests/ui/tst_HiDpi.cpp` (raw), `tst_HiDpi -functions` and `tst_Shortcuts -functions` (raw, these are the only places where `-functions` is the right command). Fix the report so that every `PASS`/`FAIL` block sits under the command that produced it, with the complete output of that run. Re-count totals by the same two commands as in the report and show the per-binary change.

## F3. Show the tests (source, not description)

`sed -n '<start>,<end>p'` (raw, whole slots) of: the Item 1 overlap test, the sibling-overlap helper, the five-control date popup test. Required properties, checked by the reviewer from the source:
- Item 1: title-only and subtitle-only painted into separate transparent images with the same fonts and the same device pixel ratio, ink rectangles compared; **not** a recomputation of the delegate's rectangles; fonts from the fix-up 7 matrix (`JOBPREP_TEST_FONT_FAMILY`), Light and Dark, 100% and 150%; states which unit each number is in (the harness line "Scale: 1.5" scales what: the font point size or `QT_SCALE_FACTOR`? say).
- Sibling check: layout-managed siblings only; **paste the hits it reports on the old code** (worktree of F1) and say for each hit whether it is a defect or an intended overlap; zero hits on the old code is an acceptable answer only if shown.
- Date test: runs inside the **real dialogs/panel**, not bare widgets; every style of `QStyleFactory::keys()`; asserts the popup is the same window the mouse click opens (`calendarWidget()->window()`), `Alt+Down`, `F4`, keypad `Alt+Down`, Esc closes only the popup and the dialog stays open, a second Esc closes the dialog, arrow keys + Enter select a date without accepting the dialog, and a case with a window-level shortcut present (Study page `Alt`+arrows) proving `ShortcutOverride` works. If any of these assertions is missing, add it (test commit) and show it failing without the corresponding production code.

## F4. `TopicDetailPanel` behaviour change

The report says `TopicDetailPanel::eventFilter` was guarded "against destroying/flushing the topic when focus temporarily shifts during calendar popup display". That is a production behaviour change (autosave and focus-out), described in one line. Show raw `git show 4d43a12 -- src/ui/widgets/TopicDetailPanel.cpp`, explain the failure it prevents with a test that fails without the guard (pop up the calendar from the panel; assert the topic is not flushed, not deleted, the detail panel keeps its state), and show that focus-out autosave still works after the popup closes (existing autosave tests plus one new slot if none covers "popup open, then close, then leave the field").

## F5. `tr()` context in `SingleTabDateTime.h`

The hint is created with `tr(...)` in a header-only class. `grep -n "Q_OBJECT" src/ui/widgets/SingleTabDateTime.h` (raw). Without `Q_OBJECT`, `tr()` resolves to the base class (`QDateEdit`/`QDateTimeEdit`), so the runtime translation context differs from the class name that `lupdate` records, and a translation of this string would never be found (a Part C bug waiting). Decide and implement one: add `Q_OBJECT` (header must be in the target's AUTOMOC sources; show it builds), or use `QCoreApplication::translate("SingleTabDateEdit", ...)` explicitly. Prove it with a test-only `QTranslator` subclass that records the context passed to `translate()`: the recorded context must be the one you chose. Also replace the four copies of the `Alt+Down`/`F4` condition (the report's grep shows lines 75, 83, 106, 117 of that header) with one helper, and show the `grep -c` before and after.

## F6. Qt facts baseline

The scratch output in §4.1 runs on the finished class (the tooltip already shows the hint). Add the baseline the amendments asked for: the same scratch test on a **plain `QDateEdit`** with `calendarPopup(true)` in `Windows` and `Fusion` (keys do not open the popup, a click at the arrow rectangle does). Raw source command and output.

## F7. Report completion

Quote the raw numbers for B0 items 8 (fixture reset timing) and 9 (`activateWindow` timeouts: count and total time) from `docs/reports/M7-partA.md` with `grep -n`, or paste them again; if they are not there, say so. Fix "Gate (ubuntu-24.04 ...)" to the real matrix names from `ci.yml`. Say what `Scale: 1.5` and `rowHeight` mean in the harness table (units). Add `docs/reports/images/` file list (`ls -la`, raw) and, in "Not verified by agent, user to check", the autosave/popup interaction of F4 and the Windows native styles (`windowsvista`, `windows11`) for the date popup test (the CI Windows job is the first place that runs them: `QStyleFactory::keys()` there).

## Finish

Raw `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` full output, totals by command, `git status --short`, `git worktree list`, `git log --oneline` of this fix-up. Report: append a section "Part B fix-up" to `docs/reports/M7-partB.md` (docs commit last).
