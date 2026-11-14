# M7 Part C fix-up: one contradiction, missing evidence, two CI risks

Read `docs/reports/M7-partC.md`, `docs/prompts/M7-partC-go.md` (its rules and tasks still apply), `AGENTS.md`.
Local commits only, never push, never tag. Never run the app binary without an isolated `HOME`/`XDG_*`. Raw output with the exact command above it, nothing trimmed (no `...`; a filter like `grep -c` is fine if the command is shown and the unfiltered `wc -l` is next to it), `(no output)` for empty, hashes only from `git log --oneline`, totals by command, no claim without a command, no hand-written lines inside raw output, no paths from tool-internal folders. Throw-away worktrees and scratch files stay outside the repository; remove worktrees and show `git worktree list` raw at the end. Do **not** start Part D.
Code commits only where a task says so; docs/report last (append a section "Part C fix-up" to `docs/reports/M7-partC.md` and correct the wrong statements listed below in place).

## Why this fix-up exists

1. **The report contradicts itself on the weekly goal.** §1 decision 7 says `7.5` = 7:50 (the user's rule: one digit after the separator = tens of minutes; 7:50 = 470 min). §C4 says `7.5 -> 465 min (7:45)` and `7,5 -> 465 min (7:45)`. 465 is 7:45. Either the test asserts a wrong value, the code regressed, or it never followed the rule.
2. Most results are stated, not shown: C0.2, C0.3 (229 hits, "100%" with no list and no per-hit verdicts), C0.6, C0.7, the sync-script failure, the presence tests, C5 ("0 violations" with no first run and no negative control), C6 outputs, C4 export results.
3. Missing from the required end block: totals by command (binaries and `-functions` lines), `git status --short`, the report commit in `git log --oneline`.
4. Two CI risks (F4, F8) and wrong CI statements in §5.

## F1. Weekly goal typing (first, it may be a regression)

Commands, raw: the typing-rule parser/validator (`grep -rn "GoalSpinBox\|parseGoal\|textFromValue\|valueFromText" src` and the `sed -n` of the functions), the committed test of the rule before Part C (`git log --oneline -S"7.75" -- tests src` and `git show <that commit>:<test file>` slot), and the table of inputs and expected minutes in `testGoalSpinBoxTypingRulesUnderLocales`. Then run the **same inputs on the old code** in a throw-away worktree of the `m6` tag (or of `0f4b9b2`, say which) and paste the raw results next to the new ones for `7.30`, `7,30`, `7.5`, `7,5`, `7.05`, `7`, `7.75`, `7.`, `.5`, `0.5` under `en`, `uk`, `ru`.
Verdict rule: the user's rule is binding: `7.30` = 450 min, `7.5` = 470 min (7:50), `7.75` is an error. If the old code gives 470 and the new gives 465, it is a Part C regression: fix it (code commit) with the failing-before/passing-after pair. If both old and new give 465, the rule was never implemented as the user stated: do **not** change behaviour silently; say so and show the evidence (the user decides). If the test asserts a value that the code does not produce, say how that test passed. Correct §1 and §C4 so they agree with the facts.

## F2. Evidence for C0 (raw, in the report or in `docs/reports/M7-partC-audit.md` if long; say where)

1. **Counts.** `grep -rnE` as in C0.1 with per-file counts (`grep -rcE ... src | grep -v ":0$"`) and `wc -l`; the same per-file counts at `0f4b9b2` (throw-away worktree) and the per-file difference, so that 482+50 vs 485+51 is explained **per file** (the report's "3 tr( including class declaration" is not evidence).
2. **C0.2.** The command that finds classes calling `tr()` without `Q_OBJECT` (not "scanned"), its raw output, and the verdict per hit; `GoalSpinBox` and `SingleTabDateEdit` are the ones you found: show the `grep -n "translate(\"GoalSpinBox\|translate(\"SingleTabDateEdit"` lines and the `tst_PageSettings::testGoalSpinBoxTranslationContext` run (raw).
3. **C0.3.** The complete raw hit list of the 229 hits (file:line:text) with one verdict per hit (UI text / not UI text). "100% are glyphs, numeric formats, URLs, QSS identifiers or settings keys" must be checkable. Then answer by command, raw: does any text produced in `src/services`, `src/domain` or `src/data` reach the UI (error messages, `EnumStrings` display, import/export status, `QSqlError` text)? List each place and how it is translated or why it is not.
4. **C0.4/C0.5.** Add the missing check from C0.4: every `tr()`/`translate()` string with two or more `%N` placeholders (raw list, word-order verdict). The static-translation grep only finds one-line statements; also show the structure of `StatusStyle`, `AgendaKindStyle` and `NotificationText` (`sed -n` of the functions that call `translate()`): are the strings translated when called (good) or stored in a static table at start-up (bad)? Say "not proven" for anything you cannot show.
5. **C0.6.** The plural table with `file:line` and the raw `sed -n` of each site (the plan listed ten sites including `ApplicationDialog.cpp:571 tr("%n min")`; your list has `TrackDialog` and does not mention that one: reconcile). Show the **English fallback helper** (raw source): when no translator is loaded, or when a catalog entry is unfinished, the helper returns clean English `1 day` / `2 days`; when a translated catalog is active it returns the catalog form. Prove with tests: `en` (no catalog) 1 and 2; `uk` with the real unfinished catalog (English shown, not `%n` or empty); a fixture catalog with a translated entry; the pseudo-translator active. Say whether every site goes through the helper (`grep -rn` for sites that call `tr("%n` directly).
6. **C0.7.** The full table of `toString(` with a format, `QLocale`, `longDayName`/`shortDayName`/`monthName`, `fromString`, `QString::number(`, `arg(` with doubles, spin boxes: place, uses system/default/neither, verdict. You changed `MonthView` and `AgendaItemDelegate` only; say why the rest (`FormatHelper`, `CalendarPage`, the date popup) needs no change, with the evidence.
7. **C0.8 mnemonics.** My go file assumed there were none; you found 16. Show the raw list. A decision "Cyrillic UI omits all `&`" needs a justification: what a Ukrainian/Russian user loses (the English `&Save` gives Alt+S), whether the Qt standard buttons from `qtbase_uk.qm`/`qtbase_ru.qm` have their own accelerators (convert with `lconvert -i <path>/qtbase_uk.qm -o <scratch>/qtbase_uk.ts` and `grep -c "&"`, raw), and the risk of two buttons sharing one accelerator letter in one dialog. Present it as a **proposal for the user** with the alternatives (no mnemonics / Cyrillic letters per dialog); the glossary doc gets a short section on it.

## F3. `en` must not depend on the system locale

Before Part C `MonthView` used a fixed English locale. Now it uses `QLocale()`. Show raw what `installLanguage` sets for each of `en`, `uk`, `ru` (`QLocale::setDefault(...)` argument). Test: set the default locale to a non-English one (for example `QLocale::German`, simulating a German system) **then** `installLanguage("en")`: `MonthView` title, weekday headers, agenda day names and the date popup show English. Show it failing on a scratch variant that does not call `setDefault` for `en`. Also complete the `resolveLanguage` table that the go file required and the report omitted: `uk`, `ru-UA`, an empty list, `["pl-PL","uk-UA"]` (state which rule you implemented: first supported entry wins, or the first entry decides), each explicit setting; show where the setting is read before any widget exists (`sed -n` of that part of `main.cpp` and `installLanguage`) and that the tests do not touch real data (isolated settings).

## F4. Translation sync check in CI (risk of a red run)

`scripts/check-translations.sh` runs `lupdate` and diffs with the committed `.ts`. CI uses other `lupdate` versions (6.8.2 and 6.10.3 from aqt, not the apt 6.10.2 of the dev machine); output differences between versions (header, ordering, plural markers) would turn CI red for a reason that is not a missing string.
1. Raw: the script; where it is called (`grep -n "check-translations" scripts/check.sh .github/workflows/ci.yml`), and whether the Windows gate runs it (it does not run `check.sh`, if I am right: show).
2. **Failing-before for the script** (required in C1.4 and not shown): add `tr("Part C sentinel")` in a scratch copy, show exit code 1 and the diff; remove it, show exit 0. Raw.
3. Decision, implement and show: the comparison **fails** only when `lupdate` is the same minor version as the one that generated the committed files (record it in the script or in a header comment of the file list, for example `JOBPREP_TS_QT_MINOR=10`); on another minor it prints `lupdate 6.8.x differs from the generating minor 6.10: report only` plus the diff and exits 0. So the first CI run of the 6.8.2 job gives data without a false red. Show both modes locally by faking the version variable.

## F5. Resources and Qt's own strings (the go file asked, the report only states)

1. `QFile::exists(u":/i18n/jobprep_uk.qm"_s)` and `jobprep_ru.qm` tests: list which test binaries contain such a test (`grep -rn "i18n/jobprep" tests`), raw, and the run output. Binaries that need the translation but have no test: add or say why not needed.
2. The `nm` line in the report is acceptable evidence that the object is linked into `jobprep`; say what it does not prove (the resource is registered at run time).
3. The "missing `qtbase_*.qm`: no warning, no crash" test: point the translations path at an empty temporary directory, run `installLanguage("uk")` under the warning guard, raw result.

## F6. C5 pseudo-translation: show that "0 violations" can be non-zero

1. Show the pseudo-translator source and one raw before/after example of strings (`%1`, `%n`, HTML, plural, `translate()` sites).
2. **Negative control:** run the same checks with the expansion raised (for example 200%) or a narrow window, and paste the raw hits; "0" at 40% means something only if the same check reports hits when text really does not fit. If the control also gives 0, the check does not look at what you think: say so and fix the check.
3. List which surfaces the run covers and which not: `ls src/ui/dialogs src/ui/widgets src/ui/pages`, raw, and mark each class as covered / not covered (candidates: `TopicDetailPanel`, preferences/data section, import/export result boxes, `Toast`/`InAppNotifier`, tray, `QMessageBox` texts, the date popup). Cover the cheap ones, list the rest.
4. Durations of the new slots measured (`-o -,xunitxml` of `ui_smoke` and `tst_HiDpi`, raw, or the per-slot lines); the "~0.4 s" and "~1.5 s" in the report have no command: replace them by numbers from the command.

## F7. C4 and C6 outputs

Paste the raw run (`ctest --preset dev-linux -R "tst_ImportExport|tst_PageSettings|tst_MonthView|tst_Translations" --output-on-failure` and each new slot with `-o -,txt` for the slot names from the report) so that the claims of C4 (names under `uk`/`ru`, exports byte-identical, backup under `uk` restored under `en`) and C6 (broken fixtures detected, plural forms) are in raw form. For the export test say what is compared (bytes of which three files) and with which helper.

## F8. Corrections of statements (in place in the report)

- §5 predictions use names and facts that do not exist: the matrix jobs are `Gate (ubuntu-latest, Qt 6.8.2)`, `Gate (ubuntu-latest, Qt 6.10.3)`, `Gate (windows-latest, Qt 6.10.3)` (use `grep -n "name:\|qt_version\|matrix" .github/workflows/ci.yml`, raw), CI installs Qt through aqt (not apt), so `/usr/share/qt6/translations` is the **dev machine**, not CI; the CI path is `<QT_PREFIX_PATH>/translations` and whether `qtbase_uk.qm` is there is unknown until the diagnostic runs. "~1.5 minutes" and "~8.5 minutes" have no source: delete them or mark them as estimates; the real earlier numbers are in the CI logs (Linux ctest about 19 s, Windows 292 s and 382 s in the last two runs).
- "Zero missing translations for end-user text" (C0.3) stays out until F2.3 is shown.
- §6: the shortcut list "`Ctrl+N`, `Ctrl+,`, `Ctrl+1..5`" must come from a command (`grep -rn "QKeySequence" src`, raw), not from memory.
- §4 says "All 35 test suites": say "35 test binaries" and give the totals by command.

## F9. Glossary must be reviewable by a busy user

`docs/i18n-glossary.md` has 321 terms; the user will not review 321 rows. Restructure: **Section 1 "Needs the user's decision"** (at most about 40 rows): domain terms that carry meaning (`Application`, `Interview`, `Follow-up`, `Target`, `Streak`, `Wishlist`, `Applied`, status and stage names, `Offer`, `Ghosted`, `Open`, `Status`, `Today`, study terms), each with uk and ru proposals, the context in the app (where the string appears) and the conflict resolved or flagged. The three known conflicts must be visible there: `Follow-up` is not "reminder"; `Application` one term in both languages (the user writes `заявка`); `Target` is a date-bound goal. **Section 2 "Mechanical"**: everything else (buttons, generic labels) in one table, to be translated by you in Part D and only spot-checked by the user. Say how many rows moved where (counts by command) and list the `//:` translator comments you added (`grep -rn "//:" src | wc -l` and the raw lines).

## Finish

Raw `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` full output, **totals by command** (binaries and `-functions` lines, compare with 34 binaries / 473 functions of Part B and name the new slots), `git status --short`, `git worktree list`, `git log --oneline` of this fix-up **including the report commit**. Then "Not verified by agent, user to check" updated (Windows: qtbase files and the language selector; shortcuts under Ukrainian and Russian layouts; the glossary decisions of Section 1; the mnemonics proposal; Cyrillic rendering on his screen).
