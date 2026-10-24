# Errata for Milestone M5 Part B Report (Round 1)

This errata document records inaccuracies and bugs present in the round 1 report (`655cdf1`) and implementation (`0ef7f91`), and details how and where each was resolved.

---

## 1. Summary of Inaccuracies and Code Issues

| # | Item / Claim in Round 1 Report (`655cdf1`) | Reality / Correct Behavior | Resolution & Commit Hash |
|---|---|---|---|
| **1** | Funnel stages described as "Applied, Screen, Technical, Offer, Accepted" with "Offer-to-Interview conversion rate" | The 5 stages correspond to ranks 1..5 defined in SPEC §6: `Applied`, `HR Screen`, `Technical`, `Final`, and `Offer`. The interview rate is `reached(rank >= 2) / reached(rank >= 1)`. The code used these stages from Part A, but the report text misstated them. | Corrected in report in `0183128`. Real widget stages and labels verified in `cbefc94`. |
| **2** | "+ Add topic" claimed to "open TopicDialog" | In M4, there is no `TopicDialog`: "+ New -> Topic" focuses the Backlog quick-add on the Study board (or opens `TrackDialog` if no tracks exist). | Test extended in `cbefc94` to assert `TrackDialog` when 0 tracks, and Study page quick-add focus without dialogs when tracks exist. Report corrected in `0183128`. |
| **3** | `DashboardController(AppContext &ctx, ...)` signature claimed | `DashboardController` takes explicit references (`StatsService&`, `AgendaService&`, `SettingsService&`, `DayWatcher&`, 5 repositories), NOT `AppContext&`. | Documented with raw grep signature in `0183128`. |
| **4** | "consistent Inter typography" claimed | Inter font is not bundled until Milestone M6; the app currently uses system UI font. | Removed mention of Inter from report in `0183128`. |
| **5** | "13 UI smoke test slots" claimed | In round 1, only 8 test slot names were explicitly listed in the report table, and slot names in the amendment table did not match B1–B8. | All 14 test slots explicitly named and mapped to `M5-partB-go.md` requirements in `0183128`. |
| **6** | Interview rate formatted as `0.7%` instead of `66.7%` | `FormatHelper::formatInterviewRate` previously called `QString::number(*rate, 'f', 1)` without multiplying by `100.0`. For `6.0 / 9.0 = 0.6666...`, it rendered `"0.7%"`. | Fixed in `src/ui/FormatHelper.cpp` in commit `cbefc94` (`tr("%1%").arg(QString::number(*rate * 100.0, 'f', 1))`). |
| **7** | Missing Part B entries in `docs/SPEC.md` §14 | SPEC §14 lacked entries for `DayWatcher`, `DashboardController`, `FormatHelper`, `GoalProgressBar`, `EmptyState` dual-action, and `MainWindow::openInterview` / `openApplication`. | Added to SPEC §14 in commit `810ab25`. |
| **8** | Unused helper overloads (dead code) | `FormatHelper` contained `formatStreakDays(int)` and `formatNextInterview(const std::optional<Domain::Interview>&, ...)`, which were never invoked by `src/ui/`. | Dead overloads removed from `FormatHelper.h` and `FormatHelper.cpp` in C0 carry-overs commit. |

---

## 2. Relevant Commits

- `0ef7f91`: Initial M5 Part B implementation
- `655cdf1`: Initial M5 Part B report
- `cbefc94`: Fix interview rate calculation (* 100.0), add GoalProgressBar helpers and ui_smoke slots
- `810ab25`: Document Part B components and navigation in `docs/SPEC.md` §14 (v1.7)
- `0183128`: Update M5 Part B report with fix-ups and accurate evidence
