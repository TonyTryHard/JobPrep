# M7 Part C: Comprehensive Localization Audit Evidence

This document provides complete, raw, verifiable evidence for the internationalization audit conducted in Milestone 7 Part C.

## 1. Per-File Call Site Counts & Differences (F2.1)

### `tr(...)` Counts

| File | `0f4b9b2` | `HEAD` | Diff | Explanation |
| :--- | :---: | :---: | :---: | :--- |
| `src/main.cpp` | 6 | 6 | +0 | Unchanged |
| `src/models/ApplicationTableModel.cpp` | 8 | 8 | +0 | Unchanged |
| `src/models/TopicListModel.cpp` | 7 | 7 | +0 | Unchanged |
| `src/ui/DataToolsController.cpp` | 54 | 54 | +0 | Unchanged |
| `src/ui/FormatHelper.cpp` | 18 | 22 | +4 | +4 plural fallback handling entries |
| `src/ui/InAppNotifier.cpp` | 1 | 1 | +0 | Unchanged |
| `src/ui/MainWindow.cpp` | 17 | 17 | +0 | Unchanged |
| `src/ui/Sidebar.cpp` | 10 | 10 | +0 | Unchanged |
| `src/ui/Toast.cpp` | 2 | 2 | +0 | Unchanged |
| `src/ui/TrayNotifier.cpp` | 2 | 2 | +0 | Unchanged |
| `src/ui/delegates/AgendaItemDelegate.cpp` | 1 | 1 | +0 | Unchanged |
| `src/ui/delegates/TopicCardDelegate.cpp` | 1 | 1 | +0 | Unchanged |
| `src/ui/dialogs/ApplicationDialog.cpp` | 68 | 68 | +0 | Unchanged |
| `src/ui/dialogs/FollowUpDialog.cpp` | 15 | 15 | +0 | Unchanged |
| `src/ui/dialogs/InterviewDialog.cpp` | 35 | 35 | +0 | Unchanged |
| `src/ui/dialogs/SessionDialog.cpp` | 14 | 14 | +0 | Unchanged |
| `src/ui/dialogs/TrackDialog.cpp` | 22 | 22 | +0 | Unchanged |
| `src/ui/pages/CalendarPage.cpp` | 28 | 28 | +0 | Unchanged |
| `src/ui/pages/HomePage.cpp` | 20 | 20 | +0 | Unchanged |
| `src/ui/pages/JobsPage.cpp` | 32 | 32 | +0 | Unchanged |
| `src/ui/pages/SettingsPage.cpp` | 59 | 61 | +2 | +4 language selector/note strings, -2 GoalSpinBox moved to translate() |
| `src/ui/pages/StudyPage.cpp` | 27 | 27 | +0 | Unchanged |
| `src/ui/widgets/FunnelWidget.cpp` | 2 | 2 | +0 | Unchanged |
| `src/ui/widgets/GoalProgressBar.cpp` | 3 | 3 | +0 | Unchanged |
| `src/ui/widgets/ProgressRing.cpp` | 1 | 1 | +0 | Unchanged |
| `src/ui/widgets/SingleTabDateTime.h` | 0 | 1 | +1 | +1 static tr() wrapper definition |
| `src/ui/widgets/TopicBoardColumn.cpp` | 7 | 7 | +0 | Unchanged |
| `src/ui/widgets/TopicDetailPanel.cpp` | 36 | 36 | +0 | Unchanged |
| **Total** | **496** | **503** | **+7** | |

### `translate(...)` Counts

| File | `0f4b9b2` | `HEAD` | Diff | Explanation |
| :--- | :---: | :---: | :---: | :--- |
| `src/ui/NotificationText.cpp` | 19 | 25 | +6 | +6 plural forms for lead times and overdue messages |
| `src/ui/delegates/AgendaItemDelegate.cpp` | 1 | 0 | -1 | -1 replaced translate() with locale.dayName() |
| `src/ui/pages/SettingsPage.cpp` | 0 | 2 | +2 | +2 explicit GoalSpinBox context translations |
| `src/ui/theme/AgendaKindStyle.cpp` | 3 | 3 | +0 | Unchanged |
| `src/ui/theme/StatusStyle.cpp` | 28 | 28 | +0 | Unchanged |
| `src/ui/widgets/SingleTabDateTime.h` | 0 | 3 | +3 | +3 explicit SingleTabDateEdit context translations |
| **Total** | **51** | **61** | **+10** | |

## 2. Context Defect Audit (F2.2)

Classes identified calling `tr()` without `Q_OBJECT` macro:
1. `GoalSpinBox` (`src/ui/pages/SettingsPage.cpp`): Inherited `QSpinBox` without `Q_OBJECT`.
   - Fixed via `QCoreApplication::translate("GoalSpinBox", ...)`
2. `SingleTabDateEdit` (`src/ui/widgets/SingleTabDateTime.h`): Inherited `QDateEdit` without `Q_OBJECT`.
   - Fixed via `QCoreApplication::translate("SingleTabDateEdit", ...)`

```
src/ui/pages/SettingsPage.cpp:124:            return QCoreApplication::translate("GoalSpinBox", "%1 h").arg(h);
src/ui/pages/SettingsPage.cpp:126:        return QCoreApplication::translate("GoalSpinBox", "%1 h %2 min").arg(h).arg(m);
src/ui/widgets/SingleTabDateTime.h:28:        return QCoreApplication::translate("SingleTabDateEdit", sourceText, disambiguation, n);
src/ui/widgets/SingleTabDateTime.h:49:            QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
src/ui/widgets/SingleTabDateTime.h:105:                QCoreApplication::translate("SingleTabDateEdit", "Press Alt+Down or F4 to open calendar");
```

## 3. Complete Audited UI String Literals (F2.3)

Total non-objectName literal occurrences audited across `src/ui`: **284**

| # | File:Line | Literal String | Category | Verdict |
| :--- | :--- | :--- | :--- | :--- |
| 1 | `src/ui/DataToolsController.cpp:203` | `yyyy-MM-dd` | Date Format | Technical ISO/formatting specifier |
| 2 | `src/ui/DataToolsController.cpp:204` | `jobprep-applications-%1.csv` | Format Specifier | Technical format string or layout template |
| 3 | `src/ui/DataToolsController.cpp:223` | `yyyy-MM-dd` | Date Format | Technical ISO/formatting specifier |
| 4 | `src/ui/DataToolsController.cpp:224` | `jobprep-interviews-%1.ics` | Format Specifier | Technical format string or layout template |
| 5 | `src/ui/DataToolsController.cpp:244` | `yyyy-MM-dd` | Date Format | Technical ISO/formatting specifier |
| 6 | `src/ui/DataToolsController.cpp:245` | `jobprep-backup-%1.json` | Format Specifier | Technical format string or layout template |
| 7 | `src/ui/DataToolsController.cpp:280` | `/backups` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 8 | `src/ui/DataToolsController.cpp:344` | `\n\nDetails: %1` | Format Specifier | Technical format string or layout template |
| 9 | `src/ui/DataToolsController.cpp:387` | `:memory:` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 10 | `src/ui/FormatHelper.cpp:35` | `%1 day(s)` | Format Specifier | Technical format string or layout template |
| 11 | `src/ui/FormatHelper.cpp:43` | `%1 hour(s)` | Format Specifier | Technical format string or layout template |
| 12 | `src/ui/FormatHelper.cpp:49` | `%1 minute(s)` | Format Specifier | Technical format string or layout template |
| 13 | `src/ui/FormatHelper.cpp:68` | `%1 active process(es)` | Format Specifier | Technical format string or layout template |
| 14 | `src/ui/FormatHelper.cpp:82` | `ddd` | Date Format | Technical ISO/formatting specifier |
| 15 | `src/ui/FormatHelper.cpp:83` | `hh:mm` | Date Format | Technical ISO/formatting specifier |
| 16 | `src/ui/InAppNotifier.cpp:153` | `QLabel { background-color: %1; border: 1px solid %2; border-radius: 6px; padding: 2px 8px; }` | CSS/Style | QSS styling rule (not user text) |
| 17 | `src/ui/MainWindow.cpp:51` | `JobPrep` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 18 | `src/ui/MainWindow.cpp:176` | `screenChanged` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 19 | `src/ui/MainWindow.cpp:179` | `show` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 20 | `src/ui/MainWindow.cpp:184` | `resize` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 21 | `src/ui/MainWindow.cpp:190` | `windowStateChange` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 22 | `src/ui/MainWindow.cpp:323` | ` | ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 23 | `src/ui/MainWindow.cpp:325` | ` | ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 24 | `src/ui/MainWindow.cpp:586` | `search` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 25 | `src/ui/MainWindow.cpp:594` | `plus` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 26 | `src/ui/NotificationText.cpp:16` | `in %1 hour(s)` | Format Specifier | Technical format string or layout template |
| 27 | `src/ui/NotificationText.cpp:26` | `in %1 minute(s)` | Format Specifier | Technical format string or layout template |
| 28 | `src/ui/NotificationText.cpp:45` | `in %1 minute(s)` | Format Specifier | Technical format string or layout template |
| 29 | `src/ui/NotificationText.cpp:55` | `HH:mm` | Date Format | Technical ISO/formatting specifier |
| 30 | `src/ui/NotificationText.cpp:59` | `HH:mm` | Date Format | Technical ISO/formatting specifier |
| 31 | `src/ui/NotificationText.cpp:65` | `in %1 hour(s)` | Format Specifier | Technical format string or layout template |
| 32 | `src/ui/NotificationText.cpp:73` | `yyyy-MM-dd HH:mm` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 33 | `src/ui/NotificationText.cpp:111` | `%1 · %2 (%3)` | Format Specifier | Technical format string or layout template |
| 34 | `src/ui/NotificationText.cpp:113` | `%1 · %2` | Format Specifier | Technical format string or layout template |
| 35 | `src/ui/NotificationText.cpp:115` | `%1 (%2)` | Format Specifier | Technical format string or layout template |
| 36 | `src/ui/NotificationText.cpp:120` | `%1 (%2)` | Format Specifier | Technical format string or layout template |
| 37 | `src/ui/NotificationText.cpp:130` | `%1 · %2` | Format Specifier | Technical format string or layout template |
| 38 | `src/ui/NotificationText.cpp:141` | `%1 — %2` | Format Specifier | Technical format string or layout template |
| 39 | `src/ui/NotificationText.cpp:151` | `%1 follow-up(s) overdue` | Format Specifier | Technical format string or layout template |
| 40 | `src/ui/Sidebar.cpp:122` | `JobPrep` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 41 | `src/ui/Sidebar.cpp:123` | `font-size: 13pt; font-weight: 700;` | CSS/Style | QSS styling rule (not user text) |
| 42 | `src/ui/Sidebar.cpp:134` | `home` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 43 | `src/ui/Sidebar.cpp:135` | `study` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 44 | `src/ui/Sidebar.cpp:136` | `jobs` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 45 | `src/ui/Sidebar.cpp:137` | `calendar` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 46 | `src/ui/Sidebar.cpp:161` | `settings` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 47 | `src/ui/Sidebar.cpp:164` | `settings` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 48 | `src/ui/Sidebar.cpp:219` | `app` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 49 | `src/ui/Sidebar.cpp:226` | `chevron-right` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 50 | `src/ui/Sidebar.cpp:226` | `chevron-left` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 51 | `src/ui/Toast.cpp:34` | `: ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 52 | `src/ui/Toast.cpp:97` | `agenda-interview` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 53 | `src/ui/Toast.cpp:100` | `agenda-followup` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 54 | `src/ui/Toast.cpp:103` | `agenda-target` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 55 | `src/ui/Toast.cpp:111` | `close` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 56 | `src/ui/TrayNotifier.cpp:75` | `app` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 57 | `src/ui/widgets/FunnelWidget.cpp:35` | `%1: %2` | Format Specifier | Technical format string or layout template |
| 58 | `src/ui/widgets/FunnelWidget.cpp:38` | `Interview rate: %1` | Format Specifier | Technical format string or layout template |
| 59 | `src/ui/widgets/FunnelWidget.cpp:39` | `, ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 60 | `src/ui/widgets/FunnelWidget.cpp:138` | `→` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 61 | `src/ui/widgets/GoalProgressBar.cpp:65` | `%1 (%2%)` | Format Specifier | Technical format string or layout template |
| 62 | `src/ui/widgets/MonthView.cpp:116` | `%1 %2` | Format Specifier | Technical format string or layout template |
| 63 | `src/ui/widgets/MonthView.cpp:118` | `MMMM yyyy` | Date Format | Technical ISO/formatting specifier |
| 64 | `src/ui/widgets/MonthView.cpp:331` | `+%1` | Format Specifier | Technical format string or layout template |
| 65 | `src/ui/widgets/ProgressRing.cpp:37` | `%1: %2%` | Format Specifier | Technical format string or layout template |
| 66 | `src/ui/widgets/ProgressRing.cpp:39` | `, ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 67 | `src/ui/widgets/ProgressRing.cpp:103` | `%1: %2%` | Format Specifier | Technical format string or layout template |
| 68 | `src/ui/widgets/ProgressRing.cpp:165` | `%1%` | Format Specifier | Technical format string or layout template |
| 69 | `src/ui/widgets/StatCard.cpp:49` | `%1: %2` | Format Specifier | Technical format string or layout template |
| 70 | `src/ui/widgets/TopicDetailPanel.cpp:54` | `http` | URL/Brand | Application title or URL scheme |
| 71 | `src/ui/widgets/TopicDetailPanel.cpp:55` | `https` | URL/Brand | Application title or URL scheme |
| 72 | `src/ui/widgets/TopicDetailPanel.cpp:146` | `yyyy-MM-dd` | Date Format | Technical ISO/formatting specifier |
| 73 | `src/ui/widgets/TopicDetailPanel.cpp:690` | `title` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 74 | `src/ui/widgets/TopicDetailPanel.cpp:691` | `url` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 75 | `src/ui/widgets/TopicDetailPanel.cpp:701` | `title` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 76 | `src/ui/widgets/TopicDetailPanel.cpp:702` | `url` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 77 | `src/ui/widgets/TopicListView.cpp:103` | `application/x-jobprep-topic-id` | URL/Brand | Application title or URL scheme |
| 78 | `src/ui/widgets/TopicListView.cpp:116` | `application/x-jobprep-topic-id` | URL/Brand | Application title or URL scheme |
| 79 | `src/ui/delegates/AgendaItemDelegate.cpp:53` | `ddd` | Date Format | Technical ISO/formatting specifier |
| 80 | `src/ui/delegates/AgendaItemDelegate.cpp:57` | ` ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 81 | `src/ui/delegates/AgendaItemDelegate.cpp:57` | `HH:mm` | Date Format | Technical ISO/formatting specifier |
| 82 | `src/ui/delegates/AgendaItemDelegate.cpp:59` | `HH:mm` | Date Format | Technical ISO/formatting specifier |
| 83 | `src/ui/delegates/ProgressBarDelegate.cpp:26` | `%` | Glyph/Separator | Visual glyph or typographical delimiter |
| 84 | `src/ui/delegates/ProgressBarDelegate.cpp:27` | `100%` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 85 | `src/ui/delegates/TopicCardDelegate.cpp:115` | `! %1` | Format Specifier | Technical format string or layout template |
| 86 | `src/ui/delegates/TopicCardDelegate.cpp:147` | `%1/%2` | Format Specifier | Technical format string or layout template |
| 87 | `src/ui/dialogs/ApplicationDialog.cpp:244` | `https://` | URL/Brand | Application title or URL scheme |
| 88 | `src/ui/dialogs/ApplicationDialog.cpp:354` | `calendar` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 89 | `src/ui/dialogs/ApplicationDialog.cpp:417` | `status` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 90 | `src/ui/dialogs/ApplicationDialog.cpp:543` | `%1 %2` | Format Specifier | Technical format string or layout template |
| 91 | `src/ui/dialogs/ApplicationDialog.cpp:544` | `HH:mm` | Date Format | Technical ISO/formatting specifier |
| 92 | `src/ui/dialogs/FollowUpDialog.cpp:112` | `%1 \u2013 %2` | Format Specifier | Technical format string or layout template |
| 93 | `src/ui/dialogs/InterviewDialog.cpp:52` | `%1 — %2` | Format Specifier | Technical format string or layout template |
| 94 | `src/ui/dialogs/InterviewDialog.cpp:104` | `HH:mm` | Date Format | Technical ISO/formatting specifier |
| 95 | `src/ui/dialogs/SessionDialog.cpp:52` | `yyyy-MM-dd` | Date Format | Technical ISO/formatting specifier |
| 96 | `src/ui/pages/CalendarPage.cpp:70` | ` \u2013 %1` | Format Specifier | Technical format string or layout template |
| 97 | `src/ui/pages/CalendarPage.cpp:141` | `chevron-left` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 98 | `src/ui/pages/CalendarPage.cpp:150` | `chevron-right` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 99 | `src/ui/pages/CalendarPage.cpp:296` | `calendar` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 100 | `src/ui/pages/CalendarPage.cpp:405` | `Agenda \u00b7 %1` | Format Specifier | Technical format string or layout template |
| 101 | `src/ui/pages/CalendarPage.cpp:406` | ` (Today)` | UI Header Text | **Defect (Fixed)**: Missing tr() on CalendarPage agenda title |
| 102 | `src/ui/pages/HomePage.cpp:92` | `0` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 103 | `src/ui/pages/HomePage.cpp:96` | `0` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 104 | `src/ui/pages/HomePage.cpp:101` | `0` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 105 | `src/ui/pages/HomePage.cpp:140` | `calendar` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 106 | `src/ui/pages/HomePage.cpp:174` | `calendar` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 107 | `src/ui/pages/HomePage.cpp:235` | `home` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 108 | `src/ui/pages/JobsPage.cpp:58` | `http` | URL/Brand | Application title or URL scheme |
| 109 | `src/ui/pages/JobsPage.cpp:58` | `https` | URL/Brand | Application title or URL scheme |
| 110 | `src/ui/pages/JobsPage.cpp:63` | `\u2013` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 111 | `src/ui/pages/JobsPage.cpp:220` | `jobs` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 112 | `src/ui/pages/JobsPage.cpp:230` | `search` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 113 | `src/ui/pages/JobsPage.cpp:373` | `%1: %2` | Format Specifier | Technical format string or layout template |
| 114 | `src/ui/pages/JobsPage.cpp:375` | `   \u00b7   ` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 115 | `src/ui/pages/SettingsPage.cpp:233` | `font-size: 12pt; font-weight: 600;` | CSS/Style | QSS styling rule (not user text) |
| 116 | `src/ui/pages/SettingsPage.cpp:242` | `system` | Setting/Token | Configuration key / internal enum token |
| 117 | `src/ui/pages/SettingsPage.cpp:243` | `light` | Setting/Token | Configuration key / internal enum token |
| 118 | `src/ui/pages/SettingsPage.cpp:244` | `dark` | Setting/Token | Configuration key / internal enum token |
| 119 | `src/ui/pages/SettingsPage.cpp:250` | `indigo` | Setting/Token | Configuration key / internal enum token |
| 120 | `src/ui/pages/SettingsPage.cpp:251` | `teal` | Setting/Token | Configuration key / internal enum token |
| 121 | `src/ui/pages/SettingsPage.cpp:252` | `rose` | Setting/Token | Configuration key / internal enum token |
| 122 | `src/ui/pages/SettingsPage.cpp:253` | `amber` | Setting/Token | Configuration key / internal enum token |
| 123 | `src/ui/pages/SettingsPage.cpp:254` | `emerald` | Setting/Token | Configuration key / internal enum token |
| 124 | `src/ui/pages/SettingsPage.cpp:255` | `sky` | Setting/Token | Configuration key / internal enum token |
| 125 | `src/ui/pages/SettingsPage.cpp:261` | `system` | Setting/Token | Configuration key / internal enum token |
| 126 | `src/ui/pages/SettingsPage.cpp:262` | `English` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 127 | `src/ui/pages/SettingsPage.cpp:262` | `en` | Setting/Token | Configuration key / internal enum token |
| 128 | `src/ui/pages/SettingsPage.cpp:263` | `uk` | Setting/Token | Configuration key / internal enum token |
| 129 | `src/ui/pages/SettingsPage.cpp:264` | `ru` | Setting/Token | Configuration key / internal enum token |
| 130 | `src/ui/pages/SettingsPage.cpp:272` | `font-size: 9pt; color: #64748b; margin-top: 4px;` | CSS/Style | QSS styling rule (not user text) |
| 131 | `src/ui/pages/SettingsPage.cpp:288` | `font-size: 12pt; font-weight: 600;` | CSS/Style | QSS styling rule (not user text) |
| 132 | `src/ui/pages/SettingsPage.cpp:318` | `font-size: 11pt; font-weight: 600;` | CSS/Style | QSS styling rule (not user text) |
| 133 | `src/ui/pages/SettingsPage.cpp:381` | `font-size: 12pt; font-weight: 600;` | CSS/Style | QSS styling rule (not user text) |
| 134 | `src/ui/pages/StudyPage.cpp:95` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 135 | `src/ui/pages/StudyPage.cpp:95` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 136 | `src/ui/pages/StudyPage.cpp:95` | `board` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 137 | `src/ui/pages/StudyPage.cpp:112` | `study` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 138 | `src/ui/pages/StudyPage.cpp:114` | `+ Topic` | UI Action Text | **Defect (Fixed)**: Missing tr() on EmptyState action button |
| 139 | `src/ui/pages/StudyPage.cpp:125` | `search` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 140 | `src/ui/pages/StudyPage.cpp:137` | `study` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 141 | `src/ui/pages/StudyPage.cpp:193` | `search` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 142 | `src/ui/pages/StudyPage.cpp:216` | `board` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 143 | `src/ui/pages/StudyPage.cpp:217` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 144 | `src/ui/pages/StudyPage.cpp:225` | `search` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 145 | `src/ui/pages/StudyPage.cpp:422` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 146 | `src/ui/pages/StudyPage.cpp:423` | `board` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 147 | `src/ui/pages/StudyPage.cpp:452` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 148 | `src/ui/pages/StudyPage.cpp:452` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 149 | `src/ui/pages/StudyPage.cpp:452` | `board` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 150 | `src/ui/pages/StudyPage.cpp:455` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 151 | `src/ui/pages/StudyPage.cpp:539` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 152 | `src/ui/pages/StudyPage.cpp:586` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 153 | `src/ui/pages/StudyPage.cpp:709` | `%1 %2` | Format Specifier | Technical format string or layout template |
| 154 | `src/ui/pages/StudyPage.cpp:849` | `list` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 155 | `src/ui/theme/AgendaKindStyle.cpp:12` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 156 | `src/ui/theme/AgendaKindStyle.cpp:14` | `#3B82F6` | CSS/Style | QSS styling rule (not user text) |
| 157 | `src/ui/theme/AgendaKindStyle.cpp:16` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 158 | `src/ui/theme/AgendaKindStyle.cpp:18` | `#3B82F6` | CSS/Style | QSS styling rule (not user text) |
| 159 | `src/ui/theme/AgendaKindStyle.cpp:36` | `agenda-interview` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 160 | `src/ui/theme/AgendaKindStyle.cpp:38` | `agenda-followup` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 161 | `src/ui/theme/AgendaKindStyle.cpp:40` | `agenda-target` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 162 | `src/ui/theme/AgendaKindStyle.cpp:42` | `agenda-interview` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 163 | `src/ui/theme/IconProvider.cpp:23` | `:/icons/` | Resource/Icon | Technical resource identifier (not user text) |
| 164 | `src/ui/theme/IconProvider.cpp:23` | `.svg` | Resource/Icon | Technical resource identifier (not user text) |
| 165 | `src/ui/theme/IconProvider.cpp:33` | `currentColor` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 166 | `src/ui/theme/StatusStyle.cpp:11` | `#8A93A6` | CSS/Style | QSS styling rule (not user text) |
| 167 | `src/ui/theme/StatusStyle.cpp:13` | `#3B82F6` | CSS/Style | QSS styling rule (not user text) |
| 168 | `src/ui/theme/StatusStyle.cpp:15` | `#8B5CF6` | CSS/Style | QSS styling rule (not user text) |
| 169 | `src/ui/theme/StatusStyle.cpp:17` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 170 | `src/ui/theme/StatusStyle.cpp:19` | `#F97316` | CSS/Style | QSS styling rule (not user text) |
| 171 | `src/ui/theme/StatusStyle.cpp:21` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 172 | `src/ui/theme/StatusStyle.cpp:23` | `#059669` | CSS/Style | QSS styling rule (not user text) |
| 173 | `src/ui/theme/StatusStyle.cpp:25` | `#EF4444` | CSS/Style | QSS styling rule (not user text) |
| 174 | `src/ui/theme/StatusStyle.cpp:27` | `#64748B` | CSS/Style | QSS styling rule (not user text) |
| 175 | `src/ui/theme/StatusStyle.cpp:29` | `#94A3B8` | CSS/Style | QSS styling rule (not user text) |
| 176 | `src/ui/theme/StatusStyle.cpp:31` | `#8A93A6` | CSS/Style | QSS styling rule (not user text) |
| 177 | `src/ui/theme/StatusStyle.cpp:64` | `#8A93A6` | CSS/Style | QSS styling rule (not user text) |
| 178 | `src/ui/theme/StatusStyle.cpp:66` | `#3B82F6` | CSS/Style | QSS styling rule (not user text) |
| 179 | `src/ui/theme/StatusStyle.cpp:68` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 180 | `src/ui/theme/StatusStyle.cpp:70` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 181 | `src/ui/theme/StatusStyle.cpp:72` | `#8A93A6` | CSS/Style | QSS styling rule (not user text) |
| 182 | `src/ui/theme/StatusStyle.cpp:92` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 183 | `src/ui/theme/StatusStyle.cpp:94` | `#3B82F6` | CSS/Style | QSS styling rule (not user text) |
| 184 | `src/ui/theme/StatusStyle.cpp:96` | `#EF4444` | CSS/Style | QSS styling rule (not user text) |
| 185 | `src/ui/theme/StatusStyle.cpp:98` | `#3B82F6` | CSS/Style | QSS styling rule (not user text) |
| 186 | `src/ui/theme/ThemeManager.cpp:78` | `fusion` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 187 | `src/ui/theme/ThemeManager.cpp:79` | `Fusion` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 188 | `src/ui/theme/ThemeManager.cpp:90` | `:/styles/theme.qss` | Resource/Icon | Technical resource identifier (not user text) |
| 189 | `src/ui/theme/ThemeManager.cpp:104` | `@bg` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 190 | `src/ui/theme/ThemeManager.cpp:105` | `@surfaceAlt` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 191 | `src/ui/theme/ThemeManager.cpp:106` | `@surface` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 192 | `src/ui/theme/ThemeManager.cpp:107` | `@border` | CSS/Style | QSS styling rule (not user text) |
| 193 | `src/ui/theme/ThemeManager.cpp:108` | `@textMuted` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 194 | `src/ui/theme/ThemeManager.cpp:109` | `@text` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 195 | `src/ui/theme/ThemeManager.cpp:110` | `@accentHover` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 196 | `src/ui/theme/ThemeManager.cpp:111` | `@accentPressed` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 197 | `src/ui/theme/ThemeManager.cpp:112` | `@accentText` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 198 | `src/ui/theme/ThemeManager.cpp:113` | `@accent` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 199 | `src/ui/theme/ThemeManager.cpp:114` | `@danger` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 200 | `src/ui/theme/ThemeManager.cpp:115` | `@warning` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 201 | `src/ui/theme/ThemeManager.cpp:116` | `@success` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 202 | `src/ui/theme/Tokens.cpp:40` | `#0F1117` | CSS/Style | QSS styling rule (not user text) |
| 203 | `src/ui/theme/Tokens.cpp:41` | `#171A23` | CSS/Style | QSS styling rule (not user text) |
| 204 | `src/ui/theme/Tokens.cpp:42` | `#1E2230` | CSS/Style | QSS styling rule (not user text) |
| 205 | `src/ui/theme/Tokens.cpp:43` | `#2A2F3F` | CSS/Style | QSS styling rule (not user text) |
| 206 | `src/ui/theme/Tokens.cpp:44` | `#E8EAF2` | CSS/Style | QSS styling rule (not user text) |
| 207 | `src/ui/theme/Tokens.cpp:45` | `#9AA3B8` | CSS/Style | QSS styling rule (not user text) |
| 208 | `src/ui/theme/Tokens.cpp:46` | `#F87171` | CSS/Style | QSS styling rule (not user text) |
| 209 | `src/ui/theme/Tokens.cpp:47` | `#FBBF24` | CSS/Style | QSS styling rule (not user text) |
| 210 | `src/ui/theme/Tokens.cpp:48` | `#34D399` | CSS/Style | QSS styling rule (not user text) |
| 211 | `src/ui/theme/Tokens.cpp:49` | `#FFFFFF` | CSS/Style | QSS styling rule (not user text) |
| 212 | `src/ui/theme/Tokens.cpp:53` | `#7C8AFF` | CSS/Style | QSS styling rule (not user text) |
| 213 | `src/ui/theme/Tokens.cpp:54` | `#939EFF` | CSS/Style | QSS styling rule (not user text) |
| 214 | `src/ui/theme/Tokens.cpp:55` | `#6774FF` | CSS/Style | QSS styling rule (not user text) |
| 215 | `src/ui/theme/Tokens.cpp:58` | `#2DD4BF` | CSS/Style | QSS styling rule (not user text) |
| 216 | `src/ui/theme/Tokens.cpp:59` | `#5EEAD4` | CSS/Style | QSS styling rule (not user text) |
| 217 | `src/ui/theme/Tokens.cpp:60` | `#14B8A6` | CSS/Style | QSS styling rule (not user text) |
| 218 | `src/ui/theme/Tokens.cpp:63` | `#FB7185` | CSS/Style | QSS styling rule (not user text) |
| 219 | `src/ui/theme/Tokens.cpp:64` | `#FDA4AF` | CSS/Style | QSS styling rule (not user text) |
| 220 | `src/ui/theme/Tokens.cpp:65` | `#F43F5E` | CSS/Style | QSS styling rule (not user text) |
| 221 | `src/ui/theme/Tokens.cpp:68` | `#FBBF24` | CSS/Style | QSS styling rule (not user text) |
| 222 | `src/ui/theme/Tokens.cpp:69` | `#FCD34D` | CSS/Style | QSS styling rule (not user text) |
| 223 | `src/ui/theme/Tokens.cpp:70` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 224 | `src/ui/theme/Tokens.cpp:73` | `#34D399` | CSS/Style | QSS styling rule (not user text) |
| 225 | `src/ui/theme/Tokens.cpp:74` | `#6EE7B7` | CSS/Style | QSS styling rule (not user text) |
| 226 | `src/ui/theme/Tokens.cpp:75` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 227 | `src/ui/theme/Tokens.cpp:78` | `#38BDF8` | CSS/Style | QSS styling rule (not user text) |
| 228 | `src/ui/theme/Tokens.cpp:79` | `#7DD3FC` | CSS/Style | QSS styling rule (not user text) |
| 229 | `src/ui/theme/Tokens.cpp:80` | `#0EA5E9` | CSS/Style | QSS styling rule (not user text) |
| 230 | `src/ui/theme/Tokens.cpp:84` | `#F6F7FB` | CSS/Style | QSS styling rule (not user text) |
| 231 | `src/ui/theme/Tokens.cpp:85` | `#FFFFFF` | CSS/Style | QSS styling rule (not user text) |
| 232 | `src/ui/theme/Tokens.cpp:86` | `#EEF0F6` | CSS/Style | QSS styling rule (not user text) |
| 233 | `src/ui/theme/Tokens.cpp:87` | `#E3E6EF` | CSS/Style | QSS styling rule (not user text) |
| 234 | `src/ui/theme/Tokens.cpp:88` | `#1F2430` | CSS/Style | QSS styling rule (not user text) |
| 235 | `src/ui/theme/Tokens.cpp:89` | `#6B7385` | CSS/Style | QSS styling rule (not user text) |
| 236 | `src/ui/theme/Tokens.cpp:90` | `#EF4444` | CSS/Style | QSS styling rule (not user text) |
| 237 | `src/ui/theme/Tokens.cpp:91` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 238 | `src/ui/theme/Tokens.cpp:92` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 239 | `src/ui/theme/Tokens.cpp:93` | `#FFFFFF` | CSS/Style | QSS styling rule (not user text) |
| 240 | `src/ui/theme/Tokens.cpp:97` | `#5B6CFF` | CSS/Style | QSS styling rule (not user text) |
| 241 | `src/ui/theme/Tokens.cpp:98` | `#4859EC` | CSS/Style | QSS styling rule (not user text) |
| 242 | `src/ui/theme/Tokens.cpp:99` | `#3A4AD6` | CSS/Style | QSS styling rule (not user text) |
| 243 | `src/ui/theme/Tokens.cpp:102` | `#14B8A6` | CSS/Style | QSS styling rule (not user text) |
| 244 | `src/ui/theme/Tokens.cpp:103` | `#0D9488` | CSS/Style | QSS styling rule (not user text) |
| 245 | `src/ui/theme/Tokens.cpp:104` | `#0F766E` | CSS/Style | QSS styling rule (not user text) |
| 246 | `src/ui/theme/Tokens.cpp:107` | `#F43F5E` | CSS/Style | QSS styling rule (not user text) |
| 247 | `src/ui/theme/Tokens.cpp:108` | `#E11D48` | CSS/Style | QSS styling rule (not user text) |
| 248 | `src/ui/theme/Tokens.cpp:109` | `#BE123C` | CSS/Style | QSS styling rule (not user text) |
| 249 | `src/ui/theme/Tokens.cpp:112` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 250 | `src/ui/theme/Tokens.cpp:113` | `#D97706` | CSS/Style | QSS styling rule (not user text) |
| 251 | `src/ui/theme/Tokens.cpp:114` | `#B45309` | CSS/Style | QSS styling rule (not user text) |
| 252 | `src/ui/theme/Tokens.cpp:117` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 253 | `src/ui/theme/Tokens.cpp:118` | `#059669` | CSS/Style | QSS styling rule (not user text) |
| 254 | `src/ui/theme/Tokens.cpp:119` | `#047857` | CSS/Style | QSS styling rule (not user text) |
| 255 | `src/ui/theme/Tokens.cpp:122` | `#0EA5E9` | CSS/Style | QSS styling rule (not user text) |
| 256 | `src/ui/theme/Tokens.cpp:123` | `#0284C7` | CSS/Style | QSS styling rule (not user text) |
| 257 | `src/ui/theme/Tokens.cpp:124` | `#0369A1` | CSS/Style | QSS styling rule (not user text) |
| 258 | `src/ui/theme/Tokens.cpp:143` | `indigo` | Setting/Token | Configuration key / internal enum token |
| 259 | `src/ui/theme/Tokens.cpp:145` | `teal` | Setting/Token | Configuration key / internal enum token |
| 260 | `src/ui/theme/Tokens.cpp:147` | `rose` | Setting/Token | Configuration key / internal enum token |
| 261 | `src/ui/theme/Tokens.cpp:149` | `amber` | Setting/Token | Configuration key / internal enum token |
| 262 | `src/ui/theme/Tokens.cpp:151` | `emerald` | Setting/Token | Configuration key / internal enum token |
| 263 | `src/ui/theme/Tokens.cpp:153` | `sky` | Setting/Token | Configuration key / internal enum token |
| 264 | `src/ui/theme/Tokens.cpp:155` | `indigo` | Setting/Token | Configuration key / internal enum token |
| 265 | `src/ui/theme/Tokens.cpp:167` | `system` | Setting/Token | Configuration key / internal enum token |
| 266 | `src/ui/theme/Tokens.cpp:169` | `light` | Setting/Token | Configuration key / internal enum token |
| 267 | `src/ui/theme/Tokens.cpp:171` | `dark` | Setting/Token | Configuration key / internal enum token |
| 268 | `src/ui/theme/Tokens.cpp:173` | `system` | Setting/Token | Configuration key / internal enum token |
| 269 | `src/ui/theme/Tokens.cpp:178` | `#5B6CFF` | CSS/Style | QSS styling rule (not user text) |
| 270 | `src/ui/theme/Tokens.cpp:178` | `Indigo` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 271 | `src/ui/theme/Tokens.cpp:179` | `#F59E0B` | CSS/Style | QSS styling rule (not user text) |
| 272 | `src/ui/theme/Tokens.cpp:179` | `Amber` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 273 | `src/ui/theme/Tokens.cpp:180` | `#0D9488` | CSS/Style | QSS styling rule (not user text) |
| 274 | `src/ui/theme/Tokens.cpp:180` | `Teal` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 275 | `src/ui/theme/Tokens.cpp:181` | `#10B981` | CSS/Style | QSS styling rule (not user text) |
| 276 | `src/ui/theme/Tokens.cpp:181` | `Emerald` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 277 | `src/ui/theme/Tokens.cpp:182` | `#F43F5E` | CSS/Style | QSS styling rule (not user text) |
| 278 | `src/ui/theme/Tokens.cpp:182` | `Rose` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 279 | `src/ui/theme/Tokens.cpp:183` | `#0284C7` | CSS/Style | QSS styling rule (not user text) |
| 280 | `src/ui/theme/Tokens.cpp:183` | `Sky` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 281 | `src/ui/theme/Tokens.cpp:184` | `#8B5CF6` | CSS/Style | QSS styling rule (not user text) |
| 282 | `src/ui/theme/Tokens.cpp:184` | `Purple` | UI Identifier/Name | Internal identifier, icon name, or constant |
| 283 | `src/ui/theme/Tokens.cpp:185` | `#F97316` | CSS/Style | QSS styling rule (not user text) |
| 284 | `src/ui/theme/Tokens.cpp:185` | `Orange` | UI Identifier/Name | Internal identifier, icon name, or constant |

## 4. Multi-Placeholder Strings (F2.4)

| File | String | Placeholders | Word-Order Verdict |
| :--- | :--- | :--- | :--- |
| `DataToolsController.cpp` | `Could not restore database:\n%1%2\n\n` | `%1`, `%2` | Positional reordering supported |
| `FormatHelper.cpp` | `%1h %2m` | `%1`, `%2` | Hours and minutes reorderable |
| `FormatHelper.cpp` | `%1 / %2` | `%1`, `%2` | Progress fraction reorderable |
| `FormatHelper.cpp` | `Next: %1 %2 %3` | `%1`, `%2`, `%3` | Interview metadata reorderable |
| `GoalProgressBar.cpp` | `%1 of %2 (%3%)` | `%1`, `%2`, `%3` | Progress numbers reorderable |
| `FollowUpDialog.cpp` | `%1 already has a follow-up planned for %2. Replace it with %3?` | `%1`, `%2`, `%3` | Company, old date, new date fully reorderable |
| `SettingsPage.cpp` | `%1 h %2 min` | `%1`, `%2` | Goal spin box units reorderable |

## 5. Plural Sites Reconciled (F2.5)

| File:Line | Source Text | Plural Handling Mechanism |
| :--- | :--- | :--- |
| `InAppNotifier.cpp:101` | `+%n more` | Direct `%n` (English 'more' invariant) |
| `ApplicationDialog.cpp:551` | `%n min` | Direct `%n` (English 'min' invariant abbreviation) |
| `TrackDialog.cpp:415` | `Delete track “%1”? This will delete %n topic(s)...` | Direct `%n` with parameterized substitution |
| `NotificationText.cpp:15` | `in %n hour(s)` | English fallback helper (`in 1 hour` / `in %1 hours`) |
| `NotificationText.cpp:25` | `in %n minute(s)` | English fallback helper (`in 1 minute` / `in %1 minutes`) |
| `NotificationText.cpp:44` | `in %n minute(s)` | English fallback helper (`in 1 minute` / `in %1 minutes`) |
| `NotificationText.cpp:64` | `in %n hour(s)` | English fallback helper (`in 1 hour` / `in %1 hours`) |
| `NotificationText.cpp:150` | `%n follow-up(s) overdue` | English fallback helper (`1 follow-up overdue` / `%1 follow-ups overdue`) |
| `FormatHelper.cpp:34` | `%n day(s)` | English fallback helper (`1 day` / `%1 days`) |
| `FormatHelper.cpp:42` | `%n hour(s)` | English fallback helper (`1 hour` / `%1 hours`) |
| `FormatHelper.cpp:48` | `%n minute(s)` | English fallback helper (`1 minute` / `%1 minutes`) |
| `FormatHelper.cpp:63` | `%n-day streak` | Direct `%n` (English hyphenated modifier invariant) |
| `FormatHelper.cpp:67` | `%n active process(es)` | English fallback helper (`1 active process` / `%1 active processes`) |
| `JobsPage.cpp:369` | `%n application(s)` | Direct `%n` (Qt pluralization) |

## 6. Raw List of 16 Accelerator Mnemonics (F2.7)

```
src/ui/dialogs/InterviewDialog.cpp:96:    form->addRow(tr("&Application *"), m_appCombo);
src/ui/dialogs/InterviewDialog.cpp:112:    form->addRow(tr("&When *"), whenRow);
src/ui/dialogs/InterviewDialog.cpp:120:    form->addRow(tr("&Duration"), m_durationSpin);
src/ui/dialogs/InterviewDialog.cpp:128:    form->addRow(tr("&Type"), m_typeCombo);
src/ui/dialogs/InterviewDialog.cpp:136:    form->addRow(tr("&Outcome"), m_outcomeCombo);
src/ui/dialogs/InterviewDialog.cpp:142:    form->addRow(tr("&Place"), m_placeEdit);
src/ui/dialogs/InterviewDialog.cpp:147:    form->addRow(tr("&Interviewer"), m_interviewerEdit);
src/ui/dialogs/InterviewDialog.cpp:154:    form->addRow(tr("&Notes"), m_notesEdit);
src/ui/dialogs/InterviewDialog.cpp:164:    m_saveBtn = buttons->addButton(tr("&Save"), QDialogButtonBox::AcceptRole);
src/ui/dialogs/InterviewDialog.cpp:171:        m_deleteBtn = buttons->addButton(tr("&Delete"), QDialogButtonBox::DestructiveRole);
src/ui/dialogs/ApplicationDialog.cpp:366:    m_addInterviewBtn = new QPushButton(tr("+ &Interview"), page);
src/ui/dialogs/ApplicationDialog.cpp:368:    m_editInterviewBtn = new QPushButton(tr("&Edit..."), page);
src/ui/dialogs/ApplicationDialog.cpp:370:    m_deleteInterviewBtn = new QPushButton(tr("&Delete..."), page);
src/ui/dialogs/FollowUpDialog.cpp:59:    form->addRow(tr("&Application *"), m_appCombo);
src/ui/dialogs/FollowUpDialog.cpp:66:    form->addRow(tr("&Action *"), m_actionEdit);
src/ui/dialogs/FollowUpDialog.cpp:82:    m_saveBtn = buttons->addButton(tr("&Save"), QDialogButtonBox::AcceptRole);
```
