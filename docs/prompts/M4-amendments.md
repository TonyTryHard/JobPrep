# M4 plan amendments (and Part A go), v2, cross-checked with SPEC 1.5

Read `AGENTS.md`, `docs/SPEC.md`, `docs/plans/M4-plan.md` and the M3 reports.
The M4 plan is **approved with the amendments below**. Do not rewrite the plan: plan + this file are the contract.
Anything that deviates from the plan, this file or the SPEC gets a SPEC §14 entry (use version **1.6**; M3 used 1.5).
Local commits only, never push. Keep prompt files out of code commits.

## Workflow change: three parts, not one commit

M4 is too big for one commit. Deliver it as three local commits, each with `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh`
green (zero warnings) and its own short report `docs/reports/M4-partA.md`, `M4-partB.md`, `M4-partC.md`
(AGENTS §10 format, including the "Not verified by agent, user to check" list).

- **Part A** (do now): plan step 1 (data, models, tests) plus the domain rule and the settings keys.
- **Part B** (later, separate go file): steps 2-3: board, DnD, keyboard, detail panel, Study page shell.
- **Part C** (later, separate go file): steps 4-6: sessions, tracks, list view, wiring, screenshots.

**Implement Part A only, write its report, and stop.** The Part B and C amendments below are binding;
read them now so Part A does not block them.

## SPEC cross-check: where the plan differs from SPEC and how it is resolved

| SPEC | Plan / decision | Resolution |
|---|---|---|
| §3.2 S6: list "grouped by track" | Flat `QTableView` with a Track column, sorted by track first | Keep the plan (confirmed decision). Add a §14 entry. |
| §7.1: header search "jumps to Jobs/Study filter" | Header search stays Jobs-only, Study has a local search box | Keep the plan (confirmed decision). Add a §14 entry. See B8. |
| §3.2 S5: due chip "red when overdue" | Done topics are never overdue (A7) | Add a §14 entry. |
| §3.2 S1: recolor | Preset swatches, no free picker | Add a §14 entry. |
| §4 / §8: `domain` = enums and structs | `topicProgress()` helper in `domain` | Add a §14 entry (derived-rule helpers may live in `domain`). |
| §3.2 S4: "Log time" button on a topic | Not in the plan | Added in B5/C1. |
| §3.2 S2: all topic fields editable | Plan does not list the track combo | Added in B5. |
| §7.3: "+ track" next to the chips, "+ Topic" toolbar button | Plan has `TrackDialog` and quick-add only | Added in B6/B9. |
| §3.2 S8: quick-add line "at the top of a column" (§7.3 mockup draws it at the bottom) | Plan: "New topic..." line | Follow the S8 text: the line is at the **top** of the column. |
| §3.2 S5: board filter chips | Plan: `ChipBar` | Keep. |
| §8 data flow: "full reset is fine for v1" | A5 | Reset is allowed, but selection/scroll/focus must survive (A5). |
| §8 errors: toast for recoverable errors | Plan has nothing | Added in B10. |
| §11: 100-200% scaling | Plan says 150% | Check 100-200% (B2). |

---

## Part A amendments (implement now)

**A1. Progress rule lives in `domain`, not in the model.** SPEC §4 defines it (subtasks -> done/total; otherwise
backlog 0, in_progress 50, review 80, done 100). D2 says "M5 StatsService can reuse the rule", but `services` must not depend
on `models`. Add a pure function in `src/domain/` (plain ints/enums only):
`int topicProgress(TopicStatus status, int doneSubtasks, int totalSubtasks)`. `TopicListModel` only calls it.
Unit test: has subtasks -> done/total; no subtasks -> 0/50/80/100 by status; total 0 and done > total are safe.
Moving a topic to Done does not tick subtasks (SPEC §4). Track progress (average, SPEC §4) is M5; do not implement it now.

**A2. Neutral model roles.** `TopicListModel` returns only neutral types: track colour as a hex `QString`
(never `QColor`), ints, `QDate`, `QString`. No `QColor/QIcon/QFont/QPainter` and no `ui/` or theme includes in `src/models/`.
Show the grep (same pattern as in the M3 fix-ups) in the report.

**A3. `TopicRepository::moveTopic` semantics (`topics.position` is one global column, SPEC §5).**
Define and test exactly this: `orderedIds` is the new order of the topics **visible** in the target column
(they may be filtered by track). Topics hidden by the filter keep their relative order and their status.
Everything is done in one transaction and emits one `changed()`. A no-op move (same status, same order) writes nothing
and emits nothing. An unknown id returns `false`, keeps `lastError()` (SPEC §8) and rolls back. Tests: move across status;
reorder inside a column; reorder in a track-filtered column does not change the relative order of hidden topics;
append at the end; unknown id.

**A4. Drop semantics in `TopicBoardProxy`.** MIME is `application/x-jobprep-topic-id` (SPEC §8). Accept only that type with
exactly one existing topic id. Drop on a card = insert before it. Drop with `row == -1`, on the empty area under the last card
or on an empty column = append. Dropping a topic on its current place is a no-op (no `changed()`).
`mimeData` round trip, `canDropMimeData` and `dropMimeData` are tested at model level. Keep `removeRows` unimplemented (finding 10).

**A5. Reload must not destroy UI state.** SPEC §8 allows a full model reset on repository `changed()`, but a subtask tick
must not make the views lose selection, scroll position or the focus in the detail panel. Pick one: row-level `dataChanged`
when the row set is the same, or a reset plus selection/scroll restore by topic id. Part A provides the model side and a test
that the row identity (topic id) is stable and that a data-only change emits `dataChanged` and no `modelReset`.
If you choose reset + restore, say so in the report and make the helper ready for Part B.

**A6. Batched progress data.** `TopicRepository::subtaskProgress()` returns done/total for all topics in one query.
The model loads topics, tracks and progress in a fixed number of queries (no N+1).

**A7. Overdue is a model fact with an injectable clock.** `TopicListModel` takes a `std::function<QDate()>` today provider
(default: real today) and exposes `OverdueRole`: target date set, before today, and status is **not** Done.
A Done topic must never look overdue (SPEC §3.2 S5 does not say this; record it in §14). Fixed-clock tests: before, on and after
today; Done is never overdue; no target date. The delegate (Part B) only paints this role.
Also check how `AgendaService` treats Done topics for study-target rows and how M5 "Today" (SPEC D2) should treat them.
Do not change `AgendaService` in M4 without a §14 entry; report what it does today.

**A8. Track changes reach the model.** The model also reloads on `TrackRepository` changes (rename, recolour, delete),
so card colours and names are never stale. Test with rename and recolour.

**A9. Settings.** Add `study/splitterState`, `study/viewMode` and `study/trackFilter` to `SettingsService` with accessors and
change signals, one definition each (check for duplicates). Reading an unknown view mode returns "board".
The fallback to "All" for a deleted track is done in the page (Part B), but add a test that the stored id is returned as is.

**A10. Part A report.** Plan vs result, hashes, grep outputs (layering, QtGui), test counts by real slot names,
SPEC §14 entries (1.6), "Not verified" list. No screenshots are needed in Part A.

---

## Part B amendments (binding, implemented later)

**B1. Keyboard access (no mouse-only features, SPEC §3.7).** DnD must have equivalents:
context menu "Move to > status", and Alt+Left/Right (change status) and Alt+Up/Down (reorder) on the selected card.
Left/Right (without Alt) moves focus between columns and keeps the row where possible; Enter opens/focuses the detail panel;
Delete deletes with confirmation (SPEC S9); Ctrl+F focuses the local Study search; Esc clears the quick-add line, and
Esc inside the detail panel (no popup open) saves and collapses it. If any shortcut collides with a menu mnemonic or an existing
shortcut, report it and choose another. Tooltips on every control.

**B2. No fixed sizes.** Card height comes from font metrics, not from pixel constants. Columns share the width with a sensible
minimum and the board scrolls horizontally when narrow. Splitter has minimum sizes and its state is remembered (D8, SPEC §3.7).
Check 100-200% scaling (SPEC §11).

**B3. Cards.** Due chip is red only when `OverdueRole` is true. Priority marker is readable without colour
(shape/text) and has a tooltip with the priority name.

**B4. Quick-add (SPEC S8).** The line is at the top of each column. Enter creates the topic in that column's status
(priority Normal) in the selected track. The placeholder/tooltip names the track ("Adds to: English"); with the "All" chip
the first track by position is used. No tracks: the field is disabled with a hint and an action "Manage tracks...".

**B5. Detail panel (SPEC S2, S3, S4, S7).** Edits all S2 fields inline: title, **track (combo)**, status, priority, target date
(clearable), notes, resources, tags. A "Log time" button opens `SessionDialog` with this topic preselected.
Checklist: tick with Space, edit, add, delete with the Delete key, reorder by drag **and** Alt+Up/Down.
Autosave on focus loss, on selection change, on page hide, on window close and on Ctrl+S. An empty title is never saved
(revert + inline message). Resource links accept only `http`/`https`, opened with `QDesktopServices` (never other schemes);
record the restriction in §14. Autosave differs from the "Discard changes?" model of `ApplicationDialog`: record this in §14 too.

**B6. Toolbar and chips (SPEC §7.3).** Chip bar `[All] [tracks...]`, a "+ track" / "Manage tracks..." button next to it
that opens `TrackDialog`, `[Board | List]` toggle and a `[+ Topic]` button that calls the same `StudyPage::newTopic()` as the header.

**B7. Header routing, direct calls only.** `+ New -> Topic` calls `StudyPage::newTopic()` (switches to Study,
focuses the quick-add of Backlog; with no tracks it opens `TrackDialog` instead). `+ New -> Study session` opens one `SessionDialog`
from any page, with the selected topic preselected when the Study page has a selection. Ctrl+N uses the current page's `triggerNew()`.
Smoke tests: exactly one dialog from Jobs and from Calendar for both entries.

**B8. Header search on the Study page.** SPEC §7.1 says the header search jumps to the Jobs/Study filter; the plan uses a
local Study search instead (confirmed decision, §14 entry). On the Study page the header search must not silently search Jobs:
disable it with a tooltip ("Use the search box on this page, Ctrl+F"), or hide it. Report which one.

**B9. Calendar jump and reuse.** `CalendarPage::studyTopicActivated(int)` -> `MainWindow` public `openTopic(int)`, which switches
to Study and selects the topic in the current view (board or list). Keep it reusable: the M5 dashboard "Today" list (SPEC D2)
will call the same `openTopic`. If the track filter or the local search hides the topic, reset them. If the topic was deleted
in the meantime, show a short message and do not crash. Test both cases.

**B10. Errors (SPEC §8).** If a repository write fails (`moveTopic`, autosave, quick-add), show a non-blocking toast
(use the existing `Toast`/message mechanism; if it does not exist yet, report that and use the nearest existing one),
reload the model so the UI shows the real data, and log with `qCWarning(jobprep.data)`.

---

## Part C amendments (binding, implemented later)

**C1. Session logging (SPEC S4).** `SessionDialog`: date defaults to today through an injectable clock, minutes >= 1 (inline
validation, schema has `CHECK (minutes > 0)`), topic optional including "(No topic)", note. SPEC requires only logging.
Do **not** add session edit/delete UI in M4; record the gap in §14 and in the report ("a wrong session cannot be corrected until M6").

**C2. `TrackDialog` (SPEC S1, S9, §5).** Add, rename, recolour (preset swatches), delete. Names are unique (inline error, use
`idByName`). Delete confirmation says how many topics will be deleted with their checklists (schema: `ON DELETE CASCADE`) and that
logged study time is kept (schema: `study_sessions.topic_id` becomes NULL). Test: after a track delete the sessions remain and
`totalMinutesInRange` is unchanged. The track filter falls back to "All" when its track is deleted. Swatches are keyboard accessible,
have colour-name tooltips, and the selected one has a visible mark (not colour alone). Record "preset swatches only" in §14.

**C3. End-to-end with Calendar.** Smoke tests: change a topic target date on Study -> Calendar shows the marker;
rename a track -> the Calendar agenda subtitle updates. Replace the old "Arrives in M4" assertion and remove the
"opens in M4"/"later milestone" tooltip text.

**C4. List view (SPEC S6).** Flat table with a Track column, default sort by track (record "not grouped" in §14).
Drag and drop is disabled in the list. Sorting uses `SortRole`: status by enum order, priority by rank, target date with empty dates
last. `StatusBadgeDelegate` keeps `Application` as the default mode. All existing Jobs tests must stay unchanged and green.

**C5. Screenshots (offscreen `QWidget::grab()`, light and dark, 100% only, no upscaled "1.5x" images):**
board, list, board with the detail panel open, empty state (no topics), empty state (no tracks), `SessionDialog`, `TrackDialog`.
List in the report every icon you added.

**C6. Final M4 report** `docs/reports/M4.md`: plan vs result across all three parts, all hashes, deviations with their
SPEC §14 entries (1.6), the M4 acceptance criteria from SPEC §12 checked one by one (dragging a card changes status and persists;
progress bars follow checklist ticks; a logged session changes the weekly minutes, tested through `totalMinutesInRange`),
"Not verified by agent, user to check" list.

---

## Expected SPEC §14 entries (1.6), collected over the three parts

1. List view is a flat sortable table with a Track column, not grouped (S6).
2. Header search is not used on the Study page; Study has a local search (SPEC §7.1).
3. Done topics are never overdue (S5).
4. Track colour: preset swatches only (S1).
5. `topicProgress()` helper lives in `domain`.
6. Detail panel autosaves (selection change, page hide, window close) and rejects an empty title.
7. Resource links: only http/https are opened.
8. No session edit/delete in M4 (S4 requires logging only).
9. `TopicRepository::moveTopic` and `subtaskProgress()`; `MainWindow::openTopic(int)`.
