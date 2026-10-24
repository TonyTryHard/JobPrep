# Milestone M5 Part A Report Errata

**Report Commit:** `d53a700` (`docs(reports): add M5 Part A report`)  
**Date:** 2026-10-24  

---

## Corrected Test Function Arithmetic

In `docs/reports/M5-partA.md` (Section 2.4), the table of test function counts was accurate across each individual test binary, but the summary arithmetic line below the table contained a clerical transcription error (`182 + 25 = 207`).

The true sum of the table rows recomputed directly from the test binaries via command:

```bash
for t in build/dev-linux/tests/{data,models,services,ui_smoke}/*; do
    if [ -x "$t" ] && [ ! -d "$t" ]; then
        "$t" -functions | wc -l
    fi
done | awk '{s+=$1} END {print "Sum of functions: " s}'
```
```
Sum of functions: 247
```

### Breakdown of True Totals

| Test Binary | Pre-M5 Count | Added in Part A | Post-Part A Count |
|---|---|---|---|
| `tst_ApplicationRepository` | 15 | 3 | 18 |
| `tst_Database` | 16 | 0 | 16 |
| `tst_InterviewRepository` | 7 | 2 | 9 |
| `tst_ReminderLogRepository` | 5 | 0 | 5 |
| `tst_SeedService` | 5 | 0 | 5 |
| `tst_SessionRepository` | 9 | 1 | 10 |
| `tst_TopicRepository` | 17 | 1 | 18 |
| `tst_TrackRepository` | 8 | 0 | 8 |
| `tst_TransactionBehavior` | 8 | 0 | 8 |
| `tst_AgendaListModel` | 5 | 0 | 5 |
| `tst_ApplicationFilterProxy` | 2 | 0 | 2 |
| `tst_ApplicationTableModel` | 6 | 0 | 6 |
| `tst_TopicBoardProxy` | 9 | 0 | 9 |
| `tst_TopicListModel` | 7 | 0 | 7 |
| `tst_TopicTableProxy` | 6 | 0 | 6 |
| `tst_AgendaService` | 15 | 9 | 24 |
| `tst_ExportService` | 4 | 0 | 4 |
| `tst_StatsService` (new) | 0 | 10 | 10 |
| `tst_MonthView` | 13 | 0 | 13 |
| `ui_smoke` | 64 | 0 | 64 |
| **Sum of Rows** | **221** | **26** | **247** |

Corrected arithmetic: **221 (pre-existing) + 26 (added in Part A) = 247 total test functions** across 20 test targets.
