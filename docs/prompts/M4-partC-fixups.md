# M4 Part C: fix-ups (report and code checks)

Read `AGENTS.md`, `docs/SPEC.md`, `docs/prompts/M4-amendments.md` (v2), `docs/prompts/M4-partC-go.md` and `docs/reports/M4-partC.md`.
Part C is **not accepted yet**. The code direction is fine, but the report skips items that `M4-partC-go.md` required
and some claims cannot be checked. Do the work below, then rewrite the report.
Local commits only, never push. Code fixes, SPEC change and the report go in separate commits.

## Report rules (same as in the go file, still strict)

1. Every command output is copied from a real run in this session, with the exact command above it. No retyped or shortened output.
2. Hashes come from `git log --oneline` after the last commit. The report is written last and committed separately.
3. Use the exact amendment names (B7, B8, B9, C1-C6) and give a `grep -n` or test-slot name as evidence for each sub-point.
4. No absolute local paths or `file:///home/...` links in committed docs. Use repo-relative paths.
5. If something is not done or not verified, say so. Do not write "fully complete".

## F1. Evidence that was required but is missing (paste raw output)

1. Layering grep: `grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models`
2. `git show --stat` for `adaa6a9`, `9680181`, `e6a69b1` and `1ebf4ee`. Confirm what each contains (SPEC change, screenshots, report, errata).
3. Test counts: `-functions | wc -l` for **every** one of the 18 test binaries. For `ui_smoke` show the arithmetic:
   Part B had 51 slots; list the slots added since then (`git diff 1ebf4ee..HEAD -- tests/ui_smoke | grep -E '^\+.*void test'`)
   and say which of `testNewEntryPointsOpenOneDialog` and `testStudyPageApi` already existed.
4. Screenshots: raw output of `./scripts/screenshots.sh`, then `file docs/reports/img/study-*.png` and `md5sum docs/reports/img/study-*.png`.
   The report shows `study-board-empty` and `study-empty-tracks` with identical sizes (24.2 KB light, 24.3 KB dark).
   If the images are identical, the "no tracks" state is not really captured: fix the test so it shows the no-tracks empty state.
5. Clean build proof of zero warnings: `cmake --build --preset dev-linux --clean-first 2>&1 | tail -30` (the earlier run said "no work to do").
6. List every icon added in M4 (C5 asked for it), or state "none".
7. The report has no "Not verified by agent, user to check" list, no "How to run" and no honest plan-vs-result gaps. Add them (AGENTS §10 format).

## F2. Code and spec checks (answer each with grep -n / slot name; fix what is wrong)

1. **QPA noise filter.** `isPlatformPluginNoise()` now ignores `"does not support raise"`. Nobody asked for that and it weakens the gate.
   Which call produces it (`raise()`, `activateWindow()`)? Is it needed in Part C at all? Remove the call or guard it so the message is not produced;
   keep the filter only if the cause is unavoidable, and then explain why in the report.
2. **Hard-coded colors.** The report lists swatches `#3b82f6, #10b981, #8b5cf6, #f59e0b, #e11d48, #6366f1, #0d9488`.
   AGENTS §7: colors come from `ui/theme/Tokens`. Run `grep -rnE "#[0-9a-fA-F]{6}" src/ui src/models src/services` and show where the palette lives.
   Move it to the theme layer if it is in the dialog. The plan said "design tokens + seeded colors": the seeded C++ (#5B6CFF) and English (#F59E0B) colors
   must be in the palette. What does `TrackDialog` show for a track whose color is not a preset? Fix and test it. Add a §14 line if the palette differs from SPEC §7.6.
3. **SessionDialog (C1).** Confirm with grep: injectable "today" clock for the default date; a "(No topic)" entry; a test that logs a session without a topic.
   Why is the maximum 1440 minutes? Keep it only if recorded in §14. A spin box with minimum 1 has no invalid state: say how "inline validation" is satisfied.
4. **TrackDialog (C2).** Show where: the delete confirmation names the **number of topics** and says logged time is kept; duplicate name error (also different case);
   swatches reachable by Tab/arrow keys, tooltip with color name, visible selected mark that is not color alone. Tests for each that is missing.
5. **Header search (B8).** The report says it is re-enabled "when switching to Jobs". Test Study -> Calendar -> Home -> Settings: the state must be correct on every page.
6. **Calendar jump (B9).** (a) Test for a topic deleted in the meantime (not only "hidden by filter"). (b) It must work in list view and board view.
   (c) The report says `MonthView` emits `studyTopicActivated` on double-click. SPEC C5 says double-click on a day opens "New interview".
   Explain exactly what the user clicks in the month view to jump to a topic, and prove with a test that C5 still works on a day that has a study target.
7. **Calendar refresh (C3).** Name the test slot that deletes a track and checks that the Calendar's study-target markers disappear. If there is none, add it.
   `CalendarPage` now takes `TrackRepository&`: add a §14 note.
8. **List view (C4).**
   - `TopicTableProxy` is not in the plan or in SPEC §8. Where is it (`src/models`?), why was it needed, layering grep clean? Add a §14 entry.
   - Default sort by track; sort by priority by rank (the report lists Track, Title, Status, Progress, Target but not Priority); empty target dates last in both directions;
     drag and drop disabled in the list. Show the code line for each.
   - Sorting rules have only UI-level coverage. Add model-level tests (new `tst_TopicTableProxy` or slots in an existing model test; update the CMake list and the count).
   - Ctrl+F, Enter and Delete work in the list view too: slot names.
9. **B7.** A test that Ctrl+N calls the current page's `triggerNew()` (Study, Jobs, Calendar). The report does not mention Ctrl+N.
10. **SPEC §14 (1.6).** Add the missing entry expected since the amendments: no session edit/delete in M4 (C1; "a wrong session cannot be corrected until M6").
    Add entries for anything else you change in F2. Check that no entry says something the code does not do.
11. Report claims "Milestone M4 is fully complete, tested, and verified". Remove it. The user sets the `m4` tag after his own manual checks.

## F3. Report

Rewrite `docs/reports/M4-partC.md` in AGENTS §10 format (What changed / How to run / How verified / Deviations and assumptions / Suggested next step),
plus: plan vs result with gaps, the amendment checklist with evidence, SPEC §12 M4 acceptance criteria one by one, hashes, and the
"Not verified by agent, user to check" list (at least: real mouse drag feel, 100-200% scaling, light/dark contrast of new widgets, tab order, calendar click-through, Wayland/X11 behavior).
Run `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` once more at the end and paste the raw output.
