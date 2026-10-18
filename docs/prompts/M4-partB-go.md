# M4 Part B: go (with Part A carry-overs)

Read `AGENTS.md`, `docs/SPEC.md`, `docs/plans/M4-plan.md`, `docs/prompts/M4-amendments.md` (v2) and `docs/reports/M4-partA.md`.
Part A is accepted. Do step B0 first as its own local commit, then Part B in one more commit. Local commits only, never push.
Keep prompt files out of code commits. Report: `docs/reports/M4-partB.md`. Stop after the report.

## B0. Carry-overs from Part A (small, one commit)

1. **Layering grep on all lower layers.** The Part A report only checked `src/models`. Run and show the output of
   `grep -rnE "QtGui|QIcon|QPixmap|QColor|QPainter|QFont" src/domain src/data src/services src/models`
   (must be empty or comments only). `TopicProgress` and `TopicRepository` are new code there.
2. **Test count.** The Part A report says `tst_MonthView` has 11 slots, but after the M3 fix-ups it has 13
   (`homeEndSelectWeekBounds`, `tabFocusEntersAndLeaves`). Check with the real binary, state the real number, and say whether any slot is missing.
3. **`lastError()` on a failed `moveTopic`.** A3 requires an unknown id to return `false` and set `lastError()`.
   Add the assertion to `moveTopicUnknownIdAndNoOp` if it is not there.
4. **Selection after a reset.** Part A reloads with `dataChanged` only when the id sequence is the same; insert, delete and reorder
   (including every DnD move) reset the model. Part B must therefore restore selection and scroll position by topic id
   (A5). Build one reusable helper in the UI layer and test it (see tests below).

## Scope of Part B

Plan steps 2-3: board, DnD, keyboard, detail panel, Study page shell. Implement amendments **B1-B6 and B10** from `M4-amendments.md`.

**Change of scope for the dialog-dependent items (decided in review):**
- `SessionDialog` and `TrackDialog` are Part C. In Part B the controls that need them ("Log time" in the detail panel,
  "Manage tracks...", the "+ track" button) exist, are **disabled** and have the tooltip "Arrives in M4 step C".
  Part C enables them and tests them.
- Header routing (`+ New -> Topic/Study session`), the header-search state on the Study page (B8) and the Calendar jump with
  `MainWindow::openTopic(int)` (B9) move to **Part C**. Do not touch the M3 "Arrives in M4" header entry or the calendar tooltip now.
- But Part B provides the page-side API that Part C will call: `StudyPage::newTopic()` (focus the Backlog quick-add; with no tracks
  do nothing visible except the hint, because `TrackDialog` is not there yet), `StudyPage::selectTopic(int id)` (clears the track filter and
  the local search if they hide the topic, selects it, returns `false` for an unknown id) and `StudyPage::triggerNew()`.
  Test them directly.

### What to build

- `ChipBar::clearChips()` and the track chips (exclusive, `All` + one per track), refreshed on `TrackRepository` changes.
  Selected chip is persisted in `study/trackFilter`. If the stored track no longer exists, fall back to `All`.
- `TopicCardDelegate`: track colour stripe (from the hex role), title, progress bar, target-date chip (red only with `OverdueRole`),
  priority marker (readable without colour, tooltip). Card height from font metrics, no pixel constants.
- `TopicBoardColumn`: header with count, quick-add line at the **top** (B4), `QListView` using `TopicBoardProxy` (one per column).
  Drag and drop between columns and inside a column, using Part A's `moveTopic`.
- `TopicDetailPanel`: all SPEC S2 fields including the track combo, checklist (tick, add, edit, delete, drag and Alt+Up/Down reorder,
  written immediately), resources table (http/https only), autosave rules from B5, "Log time" button (disabled for now).
- `StudyPage` rewrite: toolbar (chips, local search box, `[+ Topic]` calling `newTopic()`), board of four columns in a horizontally scrollable area,
  `QSplitter` with the detail panel (remembered via `study/splitterState`, collapsible). Constructor takes explicit repository/service
  references and a `std::function<QDate()>` today provider; no `AppContext&` and no singletons. `MainWindow` constructs it.
  The Board|List toggle is **not** added yet (Part C); `study/viewMode` stays unused until then.
- Keyboard (B1) and tooltips on every control; no fixed widths (B2); toast on write errors (B10).
- QSS for cards, columns, detail panel; add only icons you really use and list them in the report.

### Tests (extend `ui_smoke`, keep all existing tests unchanged and green)

- Board shows seeded topics in the right columns and track filter hides other tracks.
- Quick-add: right status, right track, placeholder names the track; with the `All` chip it uses the first track.
- Keyboard move (Alt+Right/Left) and context-menu "Move to" change the status and persist; Alt+Up/Down reorders.
- Checklist tick updates the card progress; **selection, scroll position and detail-panel focus survive it** (A5).
- Selection survives a reset (insert, delete, reorder): the restore helper is covered by a test.
- Real DnD path: send a synthesized `QDropEvent` to the column view (this exercises the row/parent values `QListView` really passes, which
  Part A only tested by calling `dropMimeData` directly). If a synthesized drop is not feasible offscreen, say so in the report and keep the model-level tests.
- Detail panel autosave on selection change; empty title is not saved and shows the inline message; target date can be cleared;
  a `javascript:`/`file:` link is not opened (only http/https).
- Delete topic with confirmation keeps its sessions (`topic_id` NULL).
- Track filter persists and falls back to `All` after the track is deleted (delete through the repository in the test).
- Overdue chip with an injected fixed clock: red before today, not red today, never red for Done.
- `newTopic()`, `selectTopic(id)` (including a hidden topic and an unknown id), `triggerNew()`.
- Both themes; zero warnings; `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` green.

## Report `docs/reports/M4-partB.md`

Plan vs result, B0 outcomes, hashes of both commits, amendment checklist (B1-B6, B10 and the moved items),
SPEC §14 entries (1.6) for new deviations, real test counts by slot name,
light and dark screenshots (offscreen `QWidget::grab()`, 100% only) of: board with cards, board with the detail panel open, empty board.
End with the "Not verified by agent, user to check" list.
