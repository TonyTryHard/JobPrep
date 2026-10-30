# M6 Part D1: fix-ups (outdated prompt was executed, evidence problems, tab-order gaps, maximize still open)

Read `AGENTS.md`, `docs/SPEC.md` (§3.6, §3.7, §11, §14), `docs/reports/M6-partD1.md`, `docs/reports/M6-partC.md`, `docs/reports/M6-partC-errata.md`, and the code named below.
Local commits only, never push. Code, SPEC and report in separate commits; the report is written last and updates `docs/reports/M6-partD1.md` in place (new section "Changes in round 2" with hashes from `git log --oneline`).
Rules as always: every output raw, exact command above it, nothing trimmed (no `...` inside pasted output), empty output written as `(no output)`, no line numbers as evidence, totals by command, no claim without a command or a named test slot.

**Why this task exists.** The D1 report shows that an **older version** of the D1 prompt was executed: C0.6 was answered with an analysis paragraph and no code change (the new typing rule is not implemented: `tst_PageSettings -functions` still lists `testGoalSpinBoxTypedDecimals` and `testGoalSpinBoxTypedFractionRounding`), the `jobprep.window` diagnostic category of C0.5 does not exist, and C0.7 has no section. The remaining points below are problems found in the report itself. Part D1 (shortcuts, empty states, Ctrl+S, backup completeness, quit during restore, `.gitattributes`, the clean proxy build) is otherwise accepted; do not redo it.

## F1. Goal spin box typing rule (user decision, implement it now)

The separator is a **time** separator, never a decimal point ("nobody mixes base 10 and base 60"). `7.30` means 7:30; `7.75` is broken syntax and must be an **error**.
- A bare integer is hours: `8` -> 480 min.
- `h<sep>mm` with `<sep>` one of `:` `;` `.` `,` and **two digits** of minutes 00-59: `7:30`, `7;30`, `7.30`, `7,30` -> 450 min; `7.05` -> 7 h 05 min.
- **One digit** after the separator is **tens of minutes**: `7.5`, `7:5`, `7,5` -> 7 h 50 min; `7.1` -> 7 h 10 min; `7.0` -> 7 h 00 min. Whoever wants 7 h 5 min types `7.05`. A digit 6-9 would mean 60-90 minutes: error.
- **Errors** (the previous value is restored on Enter and on focus loss, the code never guesses): two-digit minutes above 59 (`7.75`, `7:60`), `7.6` to `7.9`, three or more digits after the separator (`7.300`), a missing part (`7.`, `.30`), empty text, letters.
- Units stay: `8 h`, `8h`, `7 h 30 min`, `90 min` (minutes only with `min`). The goal moves in 15-minute steps (SPEC §3.6), so the parsed minutes are rounded to the nearest 15 **after** parsing, and the 60-2400 clamping stays: `7.5` (470) -> 465 (7 h 45 min), `7.05` (425) -> 420, `7.1` (430) -> 435, `7:20` -> 435, `0:50` -> 60, `99 h` and `45` -> 2400.
- Rewrite the old slots that assert decimal hours (`testGoalSpinBoxTypedDecimals`, `testGoalSpinBoxTypedFractionRounding`); no test may keep asserting the old behavior. One data-driven slot with a row per input above (value or "error"), plus a slot that checks the previous value is restored after an error. Paste the raw `-functions | grep GoalSpin` and the run.
- New SPEC §14 entry that supersedes the goal-box wording of entry 1.13. In the report show the text displayed after Enter for `7.30`, `7.5`, `7.05`, `7.75`.

## F2. Evidence integrity (answer plainly)

1. The report says `grep -n "ALTER TABLE\|user_version" src/data/Migrations.cpp` prints `(no output)`. The Part C report showed `Migrations.cpp` lines containing `PRAGMA user_version` (a read of the version and a write of the new version). Run the exact command again and paste the real output. Then say whether any other output block in `M6-partD1.md` was typed or reconstructed instead of pasted from a run; list them. An unlisted reconstruction found later counts as a failure of this task.
2. The report header says "33 binaries, 445 functions" and shows no command. Run and paste: `total=0; for b in $(find build/dev-linux/tests -maxdepth 2 -type f -executable | sort); do cnt=$("$b" -functions | wc -l); echo "$b: $cnt"; total=$((total + cnt)); done; echo "TOTAL_FUNCTIONS: $total"; echo "TOTAL_BINARIES: $(find build/dev-linux/tests -maxdepth 2 -type f -executable | wc -l)"`, and the arithmetic from the Part C baseline (31 binaries, 401 functions) per binary.
3. Paste the raw `-functions` of `tst_Shortcuts`, `tst_EmptyStatesAndFocus` and the **new** slots of `tst_PageStudy` and `ui_smoke` (`git diff 5ad0390..HEAD -- tests/ui/tst_PageStudy.cpp tests/ui_smoke/tst_UiSmoke.cpp | grep -E '^\+.*void test'`). Every slot name used in sections 3 to 5 of the report must appear in these raw lists; correct the ones that do not.
4. Paste the raw `cat docs/reports/M6-partB-errata.md` and re-map the nine C0.5 items to its sections. Item 1 of that list says "`prepareForShutdown` stops debounce timer and flushes edits ... implemented": show the code (`grep -n -A25 "void MainWindow::prepareForShutdown" src/ui/MainWindow.cpp`) and say whether it really stops timers, and which ones.

## F3. The ICS and JSON samples were not what the prompt asked for

The ICS in the report is one event from the user's own test data, cut with `...`, with no escaping, no folding, no cancelled event. Produce the dataset from the prompt with a throw-away test (not the user's database) and paste **untrimmed** `cat -A`: one interview whose notes contain a comma, a semicolon, a backslash, a newline and a Ukrainian line longer than 75 octets; one cancelled interview (must be absent); one interview without a place (no empty `LOCATION` line). Say which lines are folded and show that no folded line splits a UTF-8 sequence (for example `iconv -f UTF-8 -t UTF-8 file.ics > /dev/null; echo "exit=$?"` after unfolding with `sed ':a;N;$!ba;s/\r\n //g'`).
The JSON sample shows `"created_at": "1970-01-01T03:00:00"` and `"currency": ""` for a real application created in the app. That value is the Unix epoch in the user's time zone, so the real app wrote a bogus creation time (or the export mangles it). Find out which:
- Show where `created_at` and `updated_at` are set when an application is inserted (`grep -rn "createdAt\|updatedAt\|created_at\|updated_at" src/data/ApplicationRepository.cpp src/ui/dialogs/ApplicationDialog.cpp src/domain`), and the same for topics.
- Test (new slot, repository level): insert an application, a topic and a status change through the real repository API with a fixed clock or with "now"; assert `created_at` and `updated_at` are within a few seconds of now and not the epoch. Paste the run.
- Do not touch the user's real database. If the bug is real, fix it (own commit); existing rows are the user's problem and stay as they are (say so).
- `currency` empty: say whether the dialog writes an empty string when no salary is entered, and whether SPEC §5 (`DEFAULT 'USD'`) or the dialog is right; do not change behavior, just report.

## F4. Tab order is incomplete (D3)

The prompt required the order for every page and every tab. The report has no Study page, no sidebar, only the Overview tab of `ApplicationDialog`, the Home page only in its empty state, the Calendar page lists two layer chips (SPEC §3.4 has three: Interviews, Follow-ups, Study targets), and `InterviewDialog` shows a field `InterviewRound` and no interviewer or duration field although the schema has `interviewer` and `duration_min`. Either the chains are wrong or the report was typed from memory.
- Print the real chain for each of these from a run (a throw-away QDebug loop or the test's own failure output, raw): sidebar, Home with data and Home empty, Study (toolbar, filter chips, board columns, quick-add lines, detail panel with checklist and resources), Jobs, Calendar (all three layer chips, month view, agenda, buttons), Settings (every card), `ApplicationDialog` (Overview, Interviews, Timeline, Notes tabs), `InterviewDialog`, `FollowUpDialog`, `SessionDialog`, `TrackDialog` (including its track list).
- Replace the tab-order tables in the report and in SPEC §14 (new entry, never rewrite old ones) with the printed chains. Extend or add the tests so that each of the above is walked and compared with the documented list; name the slots from raw `-functions`.
- Any actionable control that is unreachable by Tab is a bug: fix it (own commit) and say so.
- The new `:focus` rule uses `border: 2px solid @accent`. Check in a test that focusing a push button, line edit, combo box and chip does **not** change its `geometry()` or `sizeHint()` and does not shift the text (compare before and after focus in both themes). If it does, make the unfocused border the same width (transparent or `border-color` change only) and fix it.

## F5. Maximize is still broken (the user tested the Part C build: with and without `QT_QPA_PLATFORM=xcb`)

The D1 report concluded that the problem is "compositor- or Wayland-specific". The user's test under xcb disproves that. Your own numbers show that no layout bounds the window (`maximumSize()` is 16777215 x 16777215 everywhere), which agrees with the explanation that a layout's `totalMaximumSize()` does not reach the top-level window. So the "fix" with `Qt::AlignVCenter` is unproven, and `testWindowMaximumSizeAndMaximize` passes with and without it: it proves nothing about window-manager behavior. Do this:
- Correct the report: remove the sentence that blames the compositor, state that the cause is unknown, and state that the test above checks only the Qt-side limits.
- Add a **diagnostic logging category** `jobprep.window` (debug level, off by default, enabled with `QT_LOGGING_RULES="jobprep.window.debug=true"`). On `resizeEvent`, `changeEvent(WindowStateChange)`, screen change and after `show()`, `MainWindow` logs one line: window state, `geometry()`, `frameGeometry()`, `minimumSize()`, `maximumSize()`, `minimumSizeHint()`, `sizeHint()`, `screen()->availableGeometry()`, `screen()->devicePixelRatio()`. No other behavior change. Test: the category exists and logs nothing when disabled (no new `ui_smoke` noise). The user will run the app with it, click maximize and paste the log.
- Check whether any code reacts to size or state changes by restoring a saved geometry: `grep -rn "restoreGeometry\|saveGeometry\|setGeometry\|resize(\|setMinimumSize\|setMaximumSize\|setSizeConstraint\|setWindowFlag\|windowFlags" src/ui src/main.cpp` (raw), and say if any handler can fight the window manager (for example restoring geometry on every `WindowStateChange`, or a saved geometry that is applied after `showMaximized()`).
- Do not invent another cause.

## F6. Safety backup location (C0.7 was never answered)

The user could not find `pre-restore-<timestamp>.json` after a real restore (he may have looked in the wrong folder). The `-functions` list contains `testRestoreSafetyBackupProductionPathWithCustomXdgDataHome`, but the report does not mention it. Show: the exact directory the code uses and how it is built (`organizationName`, `applicationName` as set in `src/main.cpp`), the body of that slot and a run of it, and a raw `find <temp XDG_DATA_HOME> -name "pre-restore*"` after a restore through the **production** path function (no injected directory). Show that pruning never deletes the file of the restore that just ran. Write in the report the full path pattern on Linux (`<XDG_DATA_HOME or ~/.local/share>/<org>/<app>/backups/pre-restore-<timestamp>.json`) and what the "Open data folder" button opens. If the file is not written in the real path, fix it (own commit).

## F7. Smaller points

- D2 asked for screenshots of two **new** empty states in both themes; the report lists only old images (`calendar-empty-*`, `dashboard-empty-*`, `study-board-empty-*`, `study-empty-tracks-*`). Add Jobs filtered-empty ("No matches") and the `ApplicationDialog` Interviews tab empty, light and dark, through `scripts/screenshots.sh`; raw output, `md5sum` (all different from each other and from the old ones), `git status --short`.
- The test commit `366a1fc` changed 22 tracked screenshots (the focus ring changed them; `dashboard-toasts-stack-*` grew by 9 KB and `dashboard-with-data-*` shrank by 3.5 KB). Say in one sentence per group what changed visually, or that you did not look.
- The gate output in the report again shows `ninja: no work to do`, so the gate itself did not compile anything. State in the report that the zero-warning claim rests on the proxy build of `/tmp/build-proxy-d1` and on `JOBPREP_WERROR=ON`, not on the gate run.

## Tests, checks, report

- Final `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` (raw), `git status --short`, `git log --oneline`, layering and hard-coded color greps as in the D1 prompt, and a **second proxy build** of the touched code with `-Wshadow -Wconversion -Wsign-conversion` (the previous build directory may be reused; paste the tail and the exit code of the real build, not only the second "no work to do").
- SPEC §14: new entries only (typing rule, the printed focus chains, the `jobprep.window` category, the created_at finding if real).
- "Not verified by agent, user to check": maximize with the new log (the user will send the log), typing the new goal formats in the real Settings page, Ctrl+N/F/S under the Ukrainian layout, the focus ring on the real desktop, restore on real data, everything on Windows.
