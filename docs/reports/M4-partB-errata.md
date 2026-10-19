# M4 Part B Report Errata

This document corrects three discrepancies in `docs/reports/M4-partB-fixups2.md`:

1. **Fix-Up Commit Hash:**
   - **Reported:** Section 8 reported the round-2 fix-up commit hash as `f91f0da`.
   - **Actual:** The actual commit hash on branch `main` is `1ebf4ee` (`feat(study): M4 Part B round-2 fix-ups, drag feedback and tests`).

2. **Scope of Part C Gaps:**
   - **Reported:** Section 4 listed "Study Sessions tab and Pomodoro timer (SPEC §3.3)" as deferred Part C gaps.
   - **Correction:** Neither a "Study Sessions tab" nor a Pomodoro timer is in scope for M4 or SPEC §3. A Pomodoro timer is listed in SPEC §13 ("Future / out of scope ideas"). Part C scope is strictly defined by SPEC §3.2/§3.4 and amendments B7, B8, B9, C1–C6 (`SessionDialog`, `TrackDialog`, flat list view, header routing and search, calendar jump and cascade refresh).

3. **Amendment Checklist Labels:**
   - **Reported:** Section 4 labeled items B1–B5 using class names (`TopicListModel::Role`, `SelectionRestorer`, `TopicBoardProxy`, `TopicCardDelegate`, `TopicDetailPanel`).
   - **Correction:** The checklist items must strictly use the exact canonical amendment names from `docs/prompts/M4-amendments.md`:
     - **B1 keyboard:** Context menu "Move to > status", Alt+Left/Right/Up/Down, Enter, Delete, Ctrl+F, Esc.
     - **B2 no fixed sizes:** Card height from font metrics, horizontal scroll, splitter persistence, 100–200% scaling.
     - **B3 cards:** Overdue chip red only when OverdueRole is true; priority marker readable without colour and has tooltip with priority name.
     - **B4 quick-add:** At top of column; Enter creates topic in column status; placeholder/tooltip names track; disabled when no tracks.
     - **B5 detail panel:** Inline editing of title, track combo, status, priority, target date, notes, resources, tags; subtask checklist with Space/edit/delete/reorder; autosave on focus loss/selection/hide/close/Ctrl+S; empty title rejected; http/https only for resources.
     - **B6 toolbar and chips:** Chip bar `[All] [tracks...]`, `[+ Topic]` button, `[Board | List]` toggle, "Manage tracks...".
     - **B10 errors:** Non-blocking message/toast, model reload, logged with `qCWarning(jobprep.data)`.
