# M3 fix-ups (before the `m3` tag)

Read `AGENTS.md`, `docs/SPEC.md` and `docs/reports/M3-partC.md`. Make one small local commit. Never push.
Write the answers to the questions below in `docs/reports/M3-fixups.md` (short, one section per item).

1. **Create mode (C0 item 1, missing from the Part C report).** In `ApplicationDialog`, when the application is
   not saved yet (no id), what does the Interviews tab do? `+ Interview` must be disabled with a tooltip
   ("Save the application first") or must save first. Fix it if needed and add a smoke test. Say which case it was.
2. **Stored interview time (C0 item 5).** State the exact text format the app stores for interview date-time
   (fractional seconds? offset?). Confirm the inclusive `inRange` with `23:59:59` is correct for it.
   If it is not, switch to the half-open bound from the amendments.
3. **Settings (Part A vs Part C).** Part A already added the layer-toggle settings, and Part B said
   `firstDayOfWeekChanged` already existed. The Part C report says `SettingsService` got the layer toggles in
   Part C. Explain what was already there and what is new. Make sure there is no duplicate key or duplicate API.
4. **Test counts.** The report says 34 `ui_smoke` slots (25 before + "8 new" = 33). Give the real list of the
   new slots by name and reconcile the numbers.
5. **Layering.** Run and show the output of:
   `grep -rnE "QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models`
   Only `src/ui` may use these. If `AgendaListModel` uses `QIcon` or `DecorationRole`, move it to the delegate.
6. **Keyboard.** Can `MonthView` get focus with Tab and lose it with Shift+Tab? Does it show a visible focus
   cell? Does Home/End select the first/last day of the week? Fix what is missing, test the focus part.
7. **Screenshots.** The 1.5x images are an upscale and not useful. Delete the `1.5x` files from
   `docs/reports/img/` and remove them from the report. Keep the 100% light/dark images.
8. **Go-file commit.** Do not mix prompt files into code commits from now on (just note it, no history rewrite).

Zero warnings, `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green. Add a SPEC §14 entry for any behaviour change.
