# M3 Part B — go

Part A is accepted. Continue with **Part B only** of `docs/plans/M3-plan.md`, including the Part B amendments (B1–B4) from `docs/prompts/M3-amendments.md`. Do not start Part C.

## First, confirm three small things from Part A (answer in two lines each in the Part B report; fix if needed, in the Part B commit)

1. **Boundary tests.** Does `tst_AgendaService` contain tests for an interview at `00:00:00` on `from`, at `23:59:59` on `to`, and at `00:00:00` on `to+1` (excluded)? If any is missing, add it.
2. **Fixed-clock tests.** Does `tst_AgendaService` test the `past` flag with a fixed injected clock (interview before and after "now", follow-up and target before and after "today")? If not, add it.
3. **First-day-of-week signal.** Does `SettingsService` have a change signal for the first day of week (Part C relies on it)? If not, add it with a test.

## Two decisions

- **`TrackRepository*` in `AgendaService`.** It is an optional raw pointer that was not in the plan. State in the report what it is used for (the study-target subtitle?). Change it to a required `TrackRepository&` unless there is a reason for it to be optional, so there is no null branch.
- **Meaning of `past`.** The flag is only a neutral fact. Do not dim overdue follow-ups or study targets in the UI later: an overdue item needs attention and must not look "done". Part C will dim only past interviews. Add a one-line comment on the field in `Structs.h` saying so.

## Rules (unchanged)

`CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` must pass with zero warnings before the commit. Local commit only, never push. Extend `ui_smoke` before `testZeroWarnings`. Write `docs/reports/M3-partB.md` in the AGENTS.md §10 format, with each B-amendment checked off and a "Not verified by agent, user to check" list. Run the app for real on the live display (Wayland, then `QT_QPA_PLATFORM=xcb`) with a fresh `XDG_DATA_HOME`: add an application, add an interview from the context menu, open the Interviews tab, edit and delete an interview. Then stop and report.
  Work in two commits, each only after check.sh passes: B1 = Part A confirmations + InterviewDialog + its tests; B2 = editable Interviews tab, Jobs menu, header routing, remaining tests, report. Commit B1 before starting B2. Show only the last 40 lines of build and test output; do not re-read large files you already know.
