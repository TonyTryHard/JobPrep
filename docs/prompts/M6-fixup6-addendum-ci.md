# M6 fix-up 6 addendum: the first CI run of everything up to 768aa67 (read this together with M6-fixup6.md)

This is an addendum to the task you are already doing. Same rules (raw output with the exact command above it, no trimming, local commits only, never push, never tag, do not touch `.github/workflows/`). Do your existing task 9 (font matrix) and task 10 (ApplicationDialog) first with these facts in mind, then add the items below. Put everything into `docs/reports/M6-fixup6.md` (new section "CI run of 768aa67").

## Facts (GitHub Actions, commit 768aa67, Qt 6.8.2; both jobs ran the same 34 tests)

**33 of 34 pass on Ubuntu and on Windows.** Every other test is green on both systems, including the 13 + 6 that failed before. The only red test on both is `tst_HiDpi`: `testPagesHiDpi` and `testDialogsHiDpi` fail, `testDevicePixelRatio` passes (DPI_RATIO 1.5). The failures are almost identical on the two systems, so they are not Windows-specific.

1. `testPagesHiDpi`, Page_0 (dashboard), only the Upcoming card's empty state "No upcoming interviews", in Light and Dark, at 1100x700 and at 1000x540:
   - Ubuntu (DejaVu): `EmptyStateIcon` height 32 < minimumSizeHint 48 (at 540: 36); `EmptyStateTitle` height 32 < heightForWidth 46 (at 540: 35); `EmptyStateActionBtn` height 32 < minimumSizeHint 34 (1100x700 only). 10 violations.
   - Windows: the same three widgets, 32 < 48, 32 < heightForWidth 42, 32 < 33, at both window sizes. 12 violations.
   The Today card's empty state is not in the list.
2. `testDialogsHiDpi`: 32 violations, all the same: `ApplicationDialog` (tabs 0 to 3, Light and Dark, dialog size 364x818 on Ubuntu, 330x817 on Windows), `QToolButton (ScrollLeftButton)` and `(ScrollRightButton)`: width 16 < minimumSizeHint.width 24. Those are the `QTabBar` scroll arrows. They exist only when the tab bar does not fit the dialog width.
3. On the developer machine (default font) both slots pass.

## My hypotheses (not proven, you must test them; label results as facts or "not proven")

- **Fact 2 is probably a regression from the F5 change.** The old code had `setMinimumSize(500, 420)`; the F5 replacement is `minW = fontMetrics().averageCharWidth() * 55` which gives about 364 (Ubuntu CI) and 330 (Windows) logical px: narrower than the old 500 and narrower than what the 4 tabs need with those fonts. With a narrower dialog the tab bar shows scroll arrows. The test is right to complain: a dialog that needs scroll arrows for four tabs at its own minimum size is a real defect. Do **not** whitelist the QTabBar buttons.
- **Fact 1 is probably also a side effect of F5.** Pixel minimums (190/210/250/270) were replaced by `fontMetrics().height() * n` (11, 15, 14, 6, 7), and the factors were chosen so that the test passed on the developer font. The Upcoming empty state needs icon 48 + title (2 lines possible) + button 34 + margins and spacing; the card is given less than that. The same numbers on both OS (32 px) suggest the layout squeezes the three widgets to a common height, i.e. the card/empty-state minimum is too small, not that the text is too wide.

## Tasks

A1. Reproduce both failures locally with the CI font (this is your task 9 matrix; show DejaVu at 1.5 in full) and, for Windows, with `Liberation Sans` (a metric-compatible stand-in for Arial; the CI Windows fonts are from `C:\Windows\Fonts`). If a failure does not reproduce locally, say so and what differs; do not claim a fix then.
A2. `ApplicationDialog`: compute the width the tab bar needs (`m_tabs->tabBar()->sizeHint().width()` plus the dialog margins) and show it for each of your fonts. Fix the cause: the dialog's minimum width must come from its content (the layout, and the tab bar size hint), not from a character-count multiple. Remove the arbitrary factor 55 and 24 if the layout already gives the right minimum; if an explicit minimum is still needed, derive it from `tabBar()->sizeHint()` and from `layout()->minimumSize()`. Show the old value (500), the F5 value and the new value for every font in the matrix. Then re-run `testDialogsHiDpi` for every font and scale.
A3. Dashboard empty states: find what gives the Upcoming card its height (card minimum, empty-state minimum, stretch factors, size policies, the scroll area) and why `EmptyStateIcon/Title/ActionBtn` all end up at 32 px. Show the numbers (`sizeHint()`, `minimumSizeHint()`, actual `height()` of the empty-state widget and of its card) with `qInfo` lines marked `// CI-DIAG` (temporary, remove before the final commit). Fix with layout/size policy and minimums taken from the content (`minimumSizeHint()` of the contents), not from line-count factors chosen until a test passes. Say how every remaining `fontMetrics().height() * n` factor is justified, or replace it. Re-run `testPagesHiDpi` for the whole matrix. Also confirm that the 1000x540 case still fits a 720p laptop height (<= 648) after the change.
A4. Do not weaken `tst_HiDpi`; do not add a whitelist. If you believe a violation is a test artefact (for example measured before layout activation, see your task 10), prove it with the experiment and fix the test, not the product, and say so.
A5. Because this run was the first time `tst_HiDpi` was checked on Windows with real Windows fonts, also paste from the CI log facts above what the Windows numbers imply: at which `heightForWidth` and `minimumSizeHint` the Windows font needs (42 and 33). Your fix must hold for both sets of numbers (Ubuntu 46/34, Windows 42/33).

## Report additions

In the new section: the facts above as given; for A1 to A5 the commands and raw outputs; a prediction (labelled as a prediction) of the next CI result per job. "Not verified by agent, user to check" must say: the Windows result of these two fixes (next CI run), and that the dashboard and the Application dialog look right at 150% scaling on the user's screen (user looks at both).
