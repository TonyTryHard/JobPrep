# M4 Part C: go (with Part B carry-overs)

Read `AGENTS.md`, `docs/SPEC.md`, `docs/plans/M4-plan.md`, `docs/prompts/M4-amendments.md` (v2), `docs/prompts/M4-partB-go.md`,
`docs/reports/M4-partB-fixups2.md` and the code in `src/ui/pages/StudyPage.*`.
Part B is accepted **with the carry-overs in C0**. Do C0 first as its own local commit, then Part C in one or more commits.
Local commits only, never push. Prompt files and report files go in their own commits.

## Report rules (the last two reports had invented output, so these are strict)

1. Every command output in a report is copied from a real run in this session. No retyped, shortened or "reconstructed" output.
2. Commit hashes come from `git log --oneline` after the final commit. Write the report after the code commits and commit it separately.
3. Amendment checklists use the **exact names from `M4-amendments.md`** (B1 keyboard, B2 no fixed sizes, B3 cards, B4 quick-add, B5 detail panel,
   B6 toolbar and chips, B7 header routing, B8 header search, B9 calendar jump, B10 errors, C1-C6), with a `grep -n` line for each sub-point.
4. Scope comes only from the SPEC and the amendments. Do not list things as "gaps" that the SPEC does not ask for
   (the Part B report named a Pomodoro timer and a "Study Sessions tab": neither is in the SPEC; the timer is a §13 future idea).

## C0. Carry-overs from Part B (one small commit)

1. **Layering violation.** `TopicListModel::data()` builds the priority tooltip with `Ui::Theme::StatusStyle::label(...)`.
   `src/models` must not use `ui/` or theme code. Remove the tooltip from the model; build it in the delegate (`helpEvent`) from `PriorityRole`
   and `StatusStyle`. Then run and paste the raw output of
   `grep -rnE "StatusStyle|Theme::|Tokens|ui/|QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models`
   (empty, or comments only). Add a delegate/UI test that the tooltip text is "Priority: High" etc.
2. **Drag feedback test.** The new `TopicListView` insertion line and column highlight have no test. Add a slot that sends a synthesized
   `QDragMoveEvent` and asserts that the indicator state/rectangle is set, and that it is cleared after drag leave and after drop; also the column highlight state,
   including an empty column. Say which slot writes `study-board-drag-{light,dark}.png`.
3. **Screenshots are skipped by the gate.** `testCalendarScreenshots` and `testStudyScreenshots` print SKIP when `JOBPREP_REPORT_IMG_DIR` is not set.
   Add one documented command (for example `scripts/screenshots.sh`) that sets it and regenerates all report images. Run it and paste the output.
4. **Errata for the Part B report** (a short `docs/reports/M4-partB-errata.md`): the report named the fix-up commit `f91f0da`, the real hash is `1ebf4ee`;
   the "known Part C gaps" list was wrong (see rule 4); the checklist labels B1-B5 pointed at class names, not at the amendments.
5. Confirm that `git show --stat HEAD~N` for the Part B fix-up commit contains the SPEC §14 change, the screenshots and the report.

## Part C scope

Implement amendments **B7, B8, B9, C1-C6** from `M4-amendments.md` and the plan steps 4-6. In short:

- `SessionDialog` and `TrackDialog` (C1, C2). Enable "Log time" in the detail panel, "Manage tracks..." and the `+ track` entry; the no-tracks quick-add hint now has a working action.
- Header routing (B7): `+ New -> Topic` calls `StudyPage::newTopic()` (no tracks: opens `TrackDialog`); `+ New -> Study session` opens one `SessionDialog` from any page
  with the selected topic preselected on the Study page; Ctrl+N calls the current page's `triggerNew()`. Replace the disabled "Arrives in M4" entry and update the old `ui_smoke` assertion.
- Header search on Study (B8): disabled with a tooltip or hidden; report which. It must not silently search Jobs.
- Calendar jump (B9): `CalendarPage::studyTopicActivated(int)` -> `MainWindow::openTopic(int)`; remove the "opens in M4"/"later milestone" tooltip text.
  Tests: topic hidden by the track filter or search, and topic deleted in the meantime.
- Calendar refresh (C3): topic target change and track rename reach the Calendar. **Also deleting a track** (it cascades topics) must refresh the Calendar's study-target markers,
  so the Calendar must listen to `TrackRepository::changed` as well. Add a test.
- List view (C4): `[Board | List]` toggle stored in `study/viewMode`, flat sortable table with a Track column (default sort by track), `StatusBadgeDelegate` with a `Topic` mode
  (default stays `Application`), `ProgressBarDelegate`, drag and drop disabled, sorting by `SortRole`. The detail panel, selection restore (`SelectionRestorer`),
  `selectTopic(id)`, Enter/Delete keys and the Ctrl+F search work in both views. Existing Jobs tests stay unchanged and green.
- Screenshots (C5, light and dark, 100% only): board, list, detail panel open, empty (no topics), empty (no tracks), `SessionDialog`, `TrackDialog`, board during a drag.
- Tests for everything above (named slots), zero warnings, `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green.

## Report `docs/reports/M4-partC.md` (and the final M4 summary in the same file)

Plan vs result with honest gaps; C0 outcomes with raw output; the amendment checklist with exact names; real test counts (`-functions | wc -l` for each binary);
the SPEC §12 M4 acceptance criteria checked one by one (dragging a card changes the status and persists; progress bars follow checklist ticks;
a logged session changes the weekly minutes, tested through `totalMinutesInRange`); all SPEC §14 entries (1.6) that are really committed
(flat list, header search on Study, preset swatches, no session edit/delete, `openTopic`); the commit hashes; and the "Not verified by agent, user to check" list.
