# M3 Part C: go (with Part B carry-overs)

Read `AGENTS.md`, `docs/SPEC.md`, `docs/plans/M3-plan.md`, `docs/prompts/M3-amendments.md` and the Part B report.
Part B is accepted. Do step C0 first as its own local commit. Then do Part C. Commit locally only, never push.

## C0. Carry-overs from Part B (small, one commit)

1. **Unsaved application.** What does the Interviews tab do when `ApplicationDialog` is in Create mode
   (the application has no id yet)? It must not allow `+ Interview` before the application is saved.
   Either disable the button with a tooltip that explains why, or save first. Add a smoke test. Report what you found.
2. **Layering check.** Confirm that no file in `src/models/` includes `StatusStyle` or any `ui/` or theme header.
   If one does, move the label source to a neutral place below `ui`. Put the grep command and its result in the report.
3. **Keyboard and tooltips.** `InterviewDialog` and the new Interviews tab buttons need: tooltips,
   mnemonics, a sensible tab order, Enter = Save, Esc = Cancel (with the discard confirmation), and no fixed widths.
4. **Header routing test.** Add a smoke test: `+ New -> Interview` from the Jobs page switches to Calendar
   and opens exactly one `InterviewDialog` with the selected day preselected.
5. **Interview time boundary.** `InterviewRepository::inRange` is inclusive and called with `23:59:59`.
   Check the exact text format the app stores for interview date-time (fractional seconds? offset?).
   Add a test that stores an interview at `23:59:59` exactly as the app writes it and finds it on the last day.
   If the stored format can have a suffix that breaks the text comparison, switch to the half-open bound from the amendments.
6. **SPEC §14 changelog.** Add entries for every Part A/B deviation: `TrackRepository&` is required,
   `inRange` bound behaviour, `CalendarPage` constructor with explicit refs, `StatusStyle::label(...)`, meaning of `AgendaItem::past`.
7. **Dead seams.** `m_topics`, `m_settings`, `m_theme` and `CalendarPage::hasItemsOn()` must be used by Part C
   (or removed). No unused-member warnings.

## Part C

Implement Part C exactly as in the plan with the amendments applied:

- `MonthView`: grid only. Markers and queries cover the whole **visible** grid (including leading and trailing days of the neighbour months), not only the current month.
- `AgendaListModel`: `KindRole` only. No theme or `Tokens` in `models`.
- `ui/theme/AgendaKindStyle` and `AgendaItemDelegate`.
- Dimming: only past **interviews** are dimmed. Overdue follow-ups and study targets must not look "done".
- `CalendarPage`: `QSplitter` with remembered state, real navigation widgets, layer chips bound to the layer-toggle settings, first day of week through `firstDayOfWeekChanged`.
- Real day selection: `selectedDay()` returns the selected day. Double-click on a day calls `newInterview(day)`.
- Keyboard navigation in the grid, tooltips for every control, no fixed widths.
- Tests: fixed-clock tests for anything that uses "today"; unit tests for `AgendaListModel`; smoke tests for chips, selection and double-click.
- Zero warnings (`-Werror`), `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green.

## Report `docs/reports/M3-partC.md`

- Plan versus result, amendment checklist, and the hashes of all commits (C0 and Part C).
- Light and dark theme screenshots of the Calendar page (empty, with markers, with a selected day, at 150% scale if possible).
  Use `QWidget::grab()` in the offscreen smoke run and save the images next to the report.
- Deviations (with their SPEC §14 entries).
- "Not verified by agent, user to check" list.
