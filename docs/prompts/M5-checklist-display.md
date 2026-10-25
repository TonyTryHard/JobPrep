# M5: checklist items are invisible in the real app until the theme is switched (data is saved)

Read `AGENTS.md`, `docs/SPEC.md` (§3.2 S3, S7, §7.6), `docs/reports/M5-regression.md`, `src/main.cpp`, `src/ui/theme/ThemeManager.*`, `Tokens.*`, `src/ui/widgets/TopicDetailPanel.*`, `src/ui/pages/StudyPage.*`, `resources/styles/theme.qss` (list, item and scrollbar rules), `tests/ui/tst_StudyRegression.cpp`.
Local commits only, never push. Code, SPEC and report in separate commits. Rules: raw command output only, nothing trimmed, no line numbers as evidence, no claim without a command or a test slot, totals by command with raw output.

## Facts from the user's real run (Ubuntu 26.04, GNOME, Wayland, HEAD `f91a5b9`, `jobprep.study` debug log on)
1. Adding checklist items works: the log shows `checklist add: topic= 11 len= 8` and `TopicRepository::changed` for each add, and the database has the new rows (`subtasks` ids 102 and 103 for topic 11). The board card shows the new count (`0/6`, later `0/8`). **The model is correct.**
2. The items do **not appear in the panel's checklist list**: nothing is visible after Add. Selecting another card and coming back does not help (the list is repopulated, still nothing visible).
3. **Switching the theme fixes it.** Dark -> light shows all items; switching back to light -> dark keeps them visible; a quick dark -> light -> dark also fixes it. Items added later stay visible too. After a fresh start the problem is back.
4. The text input "New checklist item" and the Add button are visible and work.
So the items exist in the widget but are **not painted visibly at startup** (stale colors, palette or style polish), and any theme change re-applies the style and repairs it. Tests missed it because they never checked what is painted for the checklist, and they do not start from a saved dark theme in the same order as `main.cpp`.

## Task 1. Find the cause (evidence, no guessing)
- Startup order: `grep -n "ThemeManager\|setMode\|apply\|SettingsService\|MainWindow" src/main.cpp` (nothing trimmed). Is the saved theme applied before or after `MainWindow`/`TopicDetailPanel` are constructed? Does applying it at startup emit the same change signal that a later user switch emits?
- Colors in the panel: `grep -n "setForeground\|setBackground\|ForegroundRole\|BackgroundRole\|tokens()\|palette\|QColor\|setStyleSheet\|themeChanged" src/ui/widgets/TopicDetailPanel.cpp`. Does the panel (or a delegate or a list item) store colors taken from the tokens **at construction time** and only refresh them on `themeChanged`? Does it connect to the theme signal at all?
- QSS: which rules style `QListWidget`, `QListWidget::item`, `QAbstractItemView` and scrollbars for the checklist (`objectName`)? Do the item text and the viewport use colors that can be equal in dark theme before the first re-apply (for example default palette text on a dark viewport)?
- Say with the code which of these is true: (a) item foreground baked from tokens that were not yet the saved theme; (b) widget not re-polished after creation (`unpolish/polish` missing for dynamically created items or for a widget whose `objectName` is set after the stylesheet was applied); (c) palette set on the app but not on this widget; (d) something else.

## Task 2. Reproduce at startup in dark theme (test must fail on HEAD)
In `tst_StudyRegression`, build the window **in the same order as `main.cpp`** with the saved theme mode set to dark (and a second run with light, and one with "System" if it can be faked) and without any later theme switch:
- A topic with 4 or more checklist items; click the card with a real mouse click; add one more through the Add button with real events.
- `QWidget::grab()` the checklist list viewport and measure painted contrast: for every row, the text pixels must differ from the viewport background with a contrast ratio of at least 3:1 (compute WCAG relative luminance on sampled pixels), and the new row must be visible (`visualItemRect` intersects the viewport).
- Then call the same theme switch the user did (dark -> light -> dark) and assert that the painted result is the same as the initial one (no difference between "fresh start" and "after switch").
Paste the failing output on HEAD. If the test passes on HEAD, change the setup until it reproduces (saved dark theme, system dark preference, startup order); do not weaken it.

## Task 3. Fix at the root and audit the same pattern
- Remove baked colors: item colors come from the widget palette/QSS at paint time, or the widget connects to the theme change signal **and** applies the current theme once after construction. No widget may depend on a theme change to look right. If the real cause is startup order (theme applied after widgets exist without a re-polish), fix `ThemeManager` so that applying a theme re-polishes all existing widgets, including the first application at startup.
- Audit: run `grep -rn "setForeground\|setBackground\|QBrush\|tokens()\.\(text\|surface\|accent\)" src/ui | grep -v "^src/ui/theme/"` and list every place that stores a token color outside the paint path or the theme handler; fix those that have the same flaw (resources table in the panel, delegates, status chips).
- Add a general startup test: with a saved dark theme and no theme switch, grab Study (board and panel with items and resources), Jobs table, Calendar agenda and Dashboard, and assert text/background contrast >= 3:1 on sampled text areas in both themes.
- Also make these improvements, each with a test: after Add the item list is refreshed from the repository and scrolled to the new row, the input is cleared and **keeps keyboard focus** (Enter adds the next item); the checklist list shows at least 5 rows, takes spare panel height, and shows a visible scrollbar when it overflows in both themes; no fixed pixel heights.
- Keep the `jobprep.study` log lines and add `checklist refreshed: count=N`. Do not change repository behavior or the autosave no-op rule.

## Task 4. Report `docs/reports/M5-checklist.md`
Facts above; code evidence from task 1 with the cause named; the failing test output on HEAD; fix commits (hashes from `git log --oneline`); new test slots by name; `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` raw output; totals by command (baseline 25 binaries, total 321); "Not verified by agent": real Wayland rendering and the first-start look at 100-200% scaling in both themes, system dark preference.
