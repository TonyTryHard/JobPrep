# JobPrep Translation Glossary & Terminology Proposals

> [!IMPORTANT]
> Every row in this document is a **draft proposal for the user's review and approval**.
> No translations have been written into `jobprep_uk.ts` or `jobprep_ru.ts` in Part C (deferred to Part D).

## 1. Terminology Decisions & Conflict Resolutions

- **`Application` (Job application):** Unified as **`Заявка`** in both Ukrainian and Russian per user specification.
- **`Follow-up` vs `Reminder`:** `Follow-up` is **`Фолоу-ап`** (uk) / **`Фоллоу-ап`** (ru). In contrast, `Reminder` refers to the system reminder service notification (**`Нагадування`** / **`Напоминание`**).
- **`Target`:** Represents a date-bound study topic goal (**`Ціль`** / **`Цель`**).
- **Disambiguated Terms:** Disambiguated in source code with `//:` translator comments and distinct contexts:
  - `Application`: menu action `tr("Application")` with comment `//: Menu action to create a new job application (not program)`
  - `Status`: column header `tr("Status")` (`//: Header for job application stage column`) vs `TopicListModel` (`//: Header for study topic status column`) vs `ApplicationDialog` (`//: Form field label for job application hiring stage`)
  - `Applied`: table header `tr("Applied")` (`//: Header for date when job application was submitted`) vs enum label `StatusStyle::label` (`//: Application hiring stage: application has been submitted`)
  - `Today`: navigation button in `CalendarPage` (`//: Button action to navigate calendar view to current day`) vs dashboard card in `HomePage` (`//: Dashboard card title showing today's agenda items`)
  - `Open`: action button in `TopicDetailPanel` (`//: Button action to open link in external web browser`)
- **Untranslated Tokens:** Program name `JobPrep`, technical acronyms `HR`, file formats `CSV`, `ICS`, `JSON` remain strictly untranslated.
- **Mnemonics:** Ukrainian and Russian translations intentionally omit all `&` keyboard mnemonics (no `&` in Cyrillic UI).

## 2. Extraction Methodology

Derived from source code via automated regex scan over all `tr()` and `translate()` calls:
```bash
python3 scripts/build_glossary.py
wc -l docs/i18n-glossary-raw.tsv  # 322 lines (321 distinct short terms + header)
```

## 3. Glossary Table (Draft Proposals for User Review)

| English UI String | Context | Ukrainian Proposal | Russian Proposal | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `Load sample data?` | `Global` | *Load sample data?* | *Load sample data?* | Proposal pending review |
| `Sample data` | `Global` | *Sample data* | *Sample data* | Proposal pending review |
| `Company` | `Global` | Компанія | Компания | Application field |
| `Position` | `Global` | Посада | Должность | Application field |
| `Status` | `Global` | Статус | Статус | Disambiguated between hiring stage and study topic |
| `Applied` | `Global` | Подано | Подано | Hiring stage: application submitted |
| `Next step` | `Global` | *Next step* | *Next step* | Proposal pending review |
| `Salary` | `Global` | Зарплата | Зарплата | Application field |
| `Source` | `Global` | *Source* | *Source* | Proposal pending review |
| `Updated` | `Global` | *Updated* | *Updated* | Proposal pending review |
| `Title` | `Global` | *Title* | *Title* | Proposal pending review |
| `Track` | `Global` | Напрям | Направление | Study track |
| `Priority` | `Global` | *Priority* | *Priority* | Proposal pending review |
| `Target Date` | `Global` | *Target Date* | *Target Date* | Proposal pending review |
| `Progress` | `Global` | *Progress* | *Progress* | Proposal pending review |
| `Tags` | `Global` | *Tags* | *Tags* | Proposal pending review |
| `No error.` | `SystemFileChooser` | *No error.* | *No error.* | Proposal pending review |
| `Export Applications (CSV)` | `SystemFileChooser` | *Export Applications (CSV)* | *Export Applications (CSV)* | Proposal pending review |
| `Export Successful` | `SystemFileChooser` | *Export Successful* | *Export Successful* | Proposal pending review |
| `Export Failed` | `SystemFileChooser` | *Export Failed* | *Export Failed* | Proposal pending review |
| `Export Interviews (ICS)` | `SystemFileChooser` | *Export Interviews (ICS)* | *Export Interviews (ICS)* | Proposal pending review |
| `Backup Database (JSON)` | `SystemFileChooser` | *Backup Database (JSON)* | *Backup Database (JSON)* | Proposal pending review |
| `Backup Successful` | `SystemFileChooser` | *Backup Successful* | *Backup Successful* | Proposal pending review |
| `Backup Failed` | `SystemFileChooser` | *Backup Failed* | *Backup Failed* | Proposal pending review |
| `Restore in Progress` | `SystemFileChooser` | *Restore in Progress* | *Restore in Progress* | Proposal pending review |
| `Confirm Database Restore` | `SystemFileChooser` | *Confirm Database Restore* | *Confirm Database Restore* | Proposal pending review |
| `Restoring Database` | `SystemFileChooser` | *Restoring Database* | *Restoring Database* | Proposal pending review |
| `Restore Successful` | `SystemFileChooser` | *Restore Successful* | *Restore Successful* | Proposal pending review |
| `Restore Failed` | `SystemFileChooser` | *Restore Failed* | *Restore Failed* | Proposal pending review |
| `Sample Data Loaded` | `SystemFileChooser` | *Sample Data Loaded* | *Sample Data Loaded* | Proposal pending review |
| `Data Folder` | `SystemFileChooser` | *Data Folder* | *Data Folder* | Proposal pending review |
| `0m` | `Global` | *0m* | *0m* | Proposal pending review |
| `0 minutes` | `Global` | *0 minutes* | *0 minutes* | Proposal pending review |
| `1 day` | `Global` | *1 day* | *1 day* | Proposal pending review |
| `1 hour` | `Global` | *1 hour* | *1 hour* | Proposal pending review |
| `1 minute` | `Global` | *1 minute* | *1 minute* | Proposal pending review |
| `1 active process` | `Global` | *1 active process* | *1 active process* | Proposal pending review |
| `JobPrep` | `Global` | JobPrep | JobPrep | Untranslated program name |
| `Search... (Ctrl+F)` | `Global` | *Search... (Ctrl+F)* | *Search... (Ctrl+F)* | Proposal pending review |
| `+ New` | `Global` | *+ New* | *+ New* | Proposal pending review |
| `+ New (Ctrl+N)` | `Global` | *+ New (Ctrl+N)* | *+ New (Ctrl+N)* | Proposal pending review |
| `0-day streak` | `Global` | *0-day streak* | *0-day streak* | Proposal pending review |
| `0 active processes` | `Global` | *0 active processes* | *0 active processes* | Proposal pending review |
| `Application` | `Global` | Заявка | Заявка | User decision: unified term for job application |
| `Interview` | `Global` | Співбесіда | Собеседование | Job interview |
| `Topic` | `Global` | Тема | Тема | Study topic |
| `Study session` | `Global` | *Study session* | *Study session* | Proposal pending review |
| `Topic not found` | `Global` | *Topic not found* | *Topic not found* | Proposal pending review |
| `Interview not found` | `Global` | *Interview not found* | *Interview not found* | Proposal pending review |
| `Application not found` | `Global` | *Application not found* | *Application not found* | Proposal pending review |
| `in %n hour(s)` | `NotificationText` | *in %n hour(s)* | *in %n hour(s)* | Proposal pending review |
| `in 1 hour` | `NotificationText` | *in 1 hour* | *in 1 hour* | Proposal pending review |
| `in %1 hours` | `NotificationText` | *in %1 hours* | *in %1 hours* | Proposal pending review |
| `in %n minute(s)` | `NotificationText` | *in %n minute(s)* | *in %n minute(s)* | Proposal pending review |
| `in 1 minute` | `NotificationText` | *in 1 minute* | *in 1 minute* | Proposal pending review |
| `in %1 minutes` | `NotificationText` | *in %1 minutes* | *in %1 minutes* | Proposal pending review |
| `today at %1` | `NotificationText` | *today at %1* | *today at %1* | Proposal pending review |
| `tomorrow at %1` | `NotificationText` | *tomorrow at %1* | *tomorrow at %1* | Proposal pending review |
| `Interview %1` | `NotificationText` | *Interview %1* | *Interview %1* | Proposal pending review |
| `Interview Reminder` | `NotificationText` | *Interview Reminder* | *Interview Reminder* | Proposal pending review |
| `Follow-up due today` | `NotificationText` | *Follow-up due today* | *Follow-up due today* | Proposal pending review |
| `Overdue Follow-up` | `NotificationText` | *Overdue Follow-up* | *Overdue Follow-up* | Proposal pending review |
| `Overdue Follow-ups` | `NotificationText` | *Overdue Follow-ups* | *Overdue Follow-ups* | Proposal pending review |
| `Interview` | `NotificationText` | Співбесіда | Собеседование | Job interview |
| `Follow-up is overdue` | `NotificationText` | *Follow-up is overdue* | *Follow-up is overdue* | Proposal pending review |
| `Dashboard` | `NavButton` | *Dashboard* | *Dashboard* | Proposal pending review |
| `Study` | `NavButton` | Навчання | Обучение | Study page |
| `Jobs` | `NavButton` | Робота | Работа | Jobs page |
| `Calendar` | `NavButton` | Календар | Календарь | Navigation page |
| `Settings` | `NavButton` | Налаштування | Настройки | Navigation page |
| `Expand sidebar` | `NavButton` | *Expand sidebar* | *Expand sidebar* | Proposal pending review |
| `Collapse sidebar` | `NavButton` | *Collapse sidebar* | *Collapse sidebar* | Proposal pending review |
| `Dismiss` | `Global` | *Dismiss* | *Dismiss* | Proposal pending review |
| `Show JobPrep` | `Global` | *Show JobPrep* | *Show JobPrep* | Proposal pending review |
| `Quit` | `Global` | *Quit* | *Quit* | Proposal pending review |
| `Overdue` | `Global` | *Overdue* | *Overdue* | Proposal pending review |
| `Priority: %1` | `Global` | *Priority: %1* | *Priority: %1* | Proposal pending review |
| `Add Application` | `TabScrollArea` | *Add Application* | *Add Application* | Proposal pending review |
| `Edit Application` | `TabScrollArea` | *Edit Application* | *Edit Application* | Proposal pending review |
| `Discard changes (Esc)` | `TabScrollArea` | *Discard changes (Esc)* | *Discard changes (Esc)* | Proposal pending review |
| `Save` | `TabScrollArea` | Зберегти | Сохранить | Primary action button |
| `Save changes (Ctrl+S)` | `TabScrollArea` | *Save changes (Ctrl+S)* | *Save changes (Ctrl+S)* | Proposal pending review |
| `Acme Corp` | `TabScrollArea` | *Acme Corp* | *Acme Corp* | Proposal pending review |
| `Company *` | `TabScrollArea` | *Company ** | *Company ** | Proposal pending review |
| `C++ Developer` | `TabScrollArea` | *C++ Developer* | *C++ Developer* | Proposal pending review |
| `Position *` | `TabScrollArea` | *Position ** | *Position ** | Proposal pending review |
| `Status` | `TabScrollArea` | Статус | Статус | Disambiguated between hiring stage and study topic |
| `Source` | `TabScrollArea` | *Source* | *Source* | Proposal pending review |
| `Work mode` | `TabScrollArea` | *Work mode* | *Work mode* | Proposal pending review |
| `not set` | `TabScrollArea` | *not set* | *not set* | Proposal pending review |
| `Salary min` | `TabScrollArea` | *Salary min* | *Salary min* | Proposal pending review |
| `Salary max` | `TabScrollArea` | *Salary max* | *Salary max* | Proposal pending review |
| `Currency` | `TabScrollArea` | *Currency* | *Currency* | Proposal pending review |
| `Set` | `TabScrollArea` | *Set* | *Set* | Proposal pending review |
| `Applied date` | `TabScrollArea` | *Applied date* | *Applied date* | Proposal pending review |
| `Next action` | `TabScrollArea` | *Next action* | *Next action* | Proposal pending review |
| `Next action date` | `TabScrollArea` | *Next action date* | *Next action date* | Proposal pending review |
| `URL` | `TabScrollArea` | *URL* | *URL* | Proposal pending review |
| `Resume version` | `TabScrollArea` | *Resume version* | *Resume version* | Proposal pending review |
| `Location` | `TabScrollArea` | Локація | Локация | Application field |
| `Contact name` | `TabScrollArea` | *Contact name* | *Contact name* | Proposal pending review |
| `Contact email` | `TabScrollArea` | *Contact email* | *Contact email* | Proposal pending review |
| `Overview` | `TabScrollArea` | *Overview* | *Overview* | Proposal pending review |
| `Notes` | `TabScrollArea` | *Notes* | *Notes* | Proposal pending review |
| `When` | `TabScrollArea` | *When* | *When* | Proposal pending review |
| `Type` | `TabScrollArea` | *Type* | *Type* | Proposal pending review |
| `Duration` | `TabScrollArea` | *Duration* | *Duration* | Proposal pending review |
| `Outcome` | `TabScrollArea` | *Outcome* | *Outcome* | Proposal pending review |
| `+ Interview` | `TabScrollArea` | *+ Interview* | *+ Interview* | Proposal pending review |
| `+ &Interview` | `TabScrollArea` | *+ Interview* | *+ Interview* | Proposal pending review |
| `&Edit...` | `TabScrollArea` | *Edit...* | *Edit...* | Proposal pending review |
| `&Delete...` | `TabScrollArea` | *Delete...* | *Delete...* | Proposal pending review |
| `Interviews` | `TabScrollArea` | Співбесіди | Собеседования | Plural of interview |
| `From` | `TabScrollArea` | *From* | *From* | Proposal pending review |
| `To` | `TabScrollArea` | *To* | *To* | Proposal pending review |
| `Timeline` | `TabScrollArea` | *Timeline* | *Timeline* | Proposal pending review |
| `—` | `TabScrollArea` | *—* | *—* | Proposal pending review |
| `Delete interview` | `TabScrollArea` | *Delete interview* | *Delete interview* | Proposal pending review |
| `Company is required.` | `TabScrollArea` | *Company is required.* | *Company is required.* | Proposal pending review |
| `Position is required.` | `TabScrollArea` | *Position is required.* | *Position is required.* | Proposal pending review |
| `Discard changes?` | `TabScrollArea` | *Discard changes?* | *Discard changes?* | Proposal pending review |
| `Add Follow-up` | `Global` | *Add Follow-up* | *Add Follow-up* | Proposal pending review |
| `&Application *` | `Global` | *Application ** | *Application ** | Proposal pending review |
| `&Action *` | `Global` | *Action ** | *Action ** | Proposal pending review |
| `Date` | `Global` | *Date* | *Date* | Proposal pending review |
| `&Save` | `Global` | Зберегти | Сохранить | Primary action button |
| `Cancel (Esc)` | `Global` | *Cancel (Esc)* | *Cancel (Esc)* | Proposal pending review |
| `Replace follow-up` | `Global` | *Replace follow-up* | *Replace follow-up* | Proposal pending review |
| `Add Interview` | `Global` | *Add Interview* | *Add Interview* | Proposal pending review |
| `Edit Interview` | `Global` | *Edit Interview* | *Edit Interview* | Proposal pending review |
| `Interview date` | `Global` | *Interview date* | *Interview date* | Proposal pending review |
| `Start time` | `Global` | *Start time* | *Start time* | Proposal pending review |
| `&When *` | `Global` | *When ** | *When ** | Proposal pending review |
| ` min` | `Global` | * min* | * min* | Proposal pending review |
| `&Duration` | `Global` | *Duration* | *Duration* | Proposal pending review |
| `&Type` | `Global` | *Type* | *Type* | Proposal pending review |
| `Result so far` | `Global` | *Result so far* | *Result so far* | Proposal pending review |
| `&Outcome` | `Global` | *Outcome* | *Outcome* | Proposal pending review |
| `Zoom, on-site address...` | `Global` | *Zoom, on-site address...* | *Zoom, on-site address...* | Proposal pending review |
| `&Place` | `Global` | *Place* | *Place* | Proposal pending review |
| `&Interviewer` | `Global` | *Interviewer* | *Interviewer* | Proposal pending review |
| `Preparation notes` | `Global` | *Preparation notes* | *Preparation notes* | Proposal pending review |
| `&Notes` | `Global` | *Notes* | *Notes* | Proposal pending review |
| `Discard changes (Esc)` | `Global` | *Discard changes (Esc)* | *Discard changes (Esc)* | Proposal pending review |
| `&Delete` | `Global` | Видалити | Удалить | Destructive action |
| `Delete this interview` | `Global` | *Delete this interview* | *Delete this interview* | Proposal pending review |
| `Discard changes?` | `Global` | *Discard changes?* | *Discard changes?* | Proposal pending review |
| `Delete interview` | `Global` | *Delete interview* | *Delete interview* | Proposal pending review |
| `Log Study Session` | `Global` | *Log Study Session* | *Log Study Session* | Proposal pending review |
| `Date:` | `Global` | *Date:* | *Date:* | Proposal pending review |
| `Duration:` | `Global` | *Duration:* | *Duration:* | Proposal pending review |
| `Topic:` | `Global` | *Topic:* | *Topic:* | Proposal pending review |
| `Notes (optional)` | `Global` | *Notes (optional)* | *Notes (optional)* | Proposal pending review |
| `Notes:` | `Global` | *Notes:* | *Notes:* | Proposal pending review |
| `Cancel` | `Global` | Скасувати | Отмена | Secondary action button |
| `Log session` | `Global` | *Log session* | *Log session* | Proposal pending review |
| `Log session (Ctrl+S)` | `Global` | *Log session (Ctrl+S)* | *Log session (Ctrl+S)* | Proposal pending review |
| `(No topic)` | `Global` | *(No topic)* | *(No topic)* | Proposal pending review |
| `Manage Tracks` | `Global` | *Manage Tracks* | *Manage Tracks* | Proposal pending review |
| `Tracks:` | `Global` | *Tracks:* | *Tracks:* | Proposal pending review |
| `+ New Track` | `Global` | *+ New Track* | *+ New Track* | Proposal pending review |
| `Name:` | `Global` | *Name:* | *Name:* | Proposal pending review |
| `Color:` | `Global` | *Color:* | *Color:* | Proposal pending review |
| `Delete Track` | `Global` | *Delete Track* | *Delete Track* | Proposal pending review |
| `Add Track` | `Global` | *Add Track* | *Add Track* | Proposal pending review |
| `Save track (Ctrl+S)` | `Global` | *Save track (Ctrl+S)* | *Save track (Ctrl+S)* | Proposal pending review |
| `Close` | `Global` | Закрити | Закрыть | Close dialog or panel |
| `Close dialog (Esc)` | `Global` | *Close dialog (Esc)* | *Close dialog (Esc)* | Proposal pending review |
| `Custom (%1)` | `Global` | *Custom (%1)* | *Custom (%1)* | Proposal pending review |
| `Save Track` | `Global` | *Save Track* | *Save Track* | Proposal pending review |
| `Calendar` | `Global` | Календар | Календарь | Navigation page |
| `Previous month` | `Global` | *Previous month* | *Previous month* | Proposal pending review |
| `Next month` | `Global` | *Next month* | *Next month* | Proposal pending review |
| `Today` | `Global` | Сьогодні | Сегодня | Current day |
| `Go to today` | `Global` | *Go to today* | *Go to today* | Proposal pending review |
| `Interviews` | `Global` | Співбесіди | Собеседования | Plural of interview |
| `Follow-ups` | `Global` | Фолоу-апи | Фоллоу-апы | Plural of Follow-up |
| `Study targets` | `Global` | Навчальні цілі | Учебные цели | Layer chip in Calendar |
| `+ Interview` | `Global` | *+ Interview* | *+ Interview* | Proposal pending review |
| `+ Interview (Ctrl+N)` | `Global` | *+ Interview (Ctrl+N)* | *+ Interview (Ctrl+N)* | Proposal pending review |
| `+ Follow-up` | `Global` | *+ Follow-up* | *+ Follow-up* | Proposal pending review |
| `+ Study target` | `Global` | *+ Study target* | *+ Study target* | Proposal pending review |
| `Nothing Planned` | `Global` | *Nothing Planned* | *Nothing Planned* | Proposal pending review |
| `Agenda` | `Global` | *Agenda* | *Agenda* | Proposal pending review |
| `No Day Selected` | `Global` | *No Day Selected* | *No Day Selected* | Proposal pending review |
| `Dashboard` | `Global` | *Dashboard* | *Dashboard* | Proposal pending review |
| `Applied this week` | `Global` | *Applied this week* | *Applied this week* | Proposal pending review |
| `Active processes` | `Global` | *Active processes* | *Active processes* | Proposal pending review |
| `Upcoming interviews` | `Global` | *Upcoming interviews* | *Upcoming interviews* | Proposal pending review |
| `Next 7 days` | `Global` | *Next 7 days* | *Next 7 days* | Proposal pending review |
| `Study streak` | `Global` | *Study streak* | *Study streak* | Proposal pending review |
| `No upcoming interviews` | `Global` | *No upcoming interviews* | *No upcoming interviews* | Proposal pending review |
| `+ Schedule interview` | `Global` | *+ Schedule interview* | *+ Schedule interview* | Proposal pending review |
| `Study progress` | `Global` | *Study progress* | *Study progress* | Proposal pending review |
| `Hiring funnel` | `Global` | *Hiring funnel* | *Hiring funnel* | Proposal pending review |
| `No Activity Yet` | `Global` | *No Activity Yet* | *No Activity Yet* | Proposal pending review |
| `+ Add application` | `Global` | *+ Add application* | *+ Add application* | Proposal pending review |
| `+ Add topic` | `Global` | *+ Add topic* | *+ Add topic* | Proposal pending review |
| `Job Applications` | `Global` | Заявки на роботу | Заявки на работу | Section title |
| `All` | `Global` | *All* | *All* | Proposal pending review |
| `Active` | `Global` | *Active* | *Active* | Proposal pending review |
| `Interviewing` | `Global` | *Interviewing* | *Interviewing* | Proposal pending review |
| `Offer` | `Global` | Офер | Оффер | Hiring stage |
| `Closed` | `Global` | *Closed* | *Closed* | Proposal pending review |
| `Export CSV` | `Global` | *Export CSV* | *Export CSV* | Proposal pending review |
| `+ Application` | `Global` | *+ Application* | *+ Application* | Proposal pending review |
| `CSV files (*.csv)` | `Global` | *CSV files (*.csv)* | *CSV files (*.csv)* | Proposal pending review |
| `Export failed` | `Global` | *Export failed* | *Export failed* | Proposal pending review |
| `No Applications Yet` | `Global` | *No Applications Yet* | *No Applications Yet* | Proposal pending review |
| `No Matches` | `Global` | *No Matches* | *No Matches* | Proposal pending review |
| `Clear filters` | `Global` | *Clear filters* | *Clear filters* | Proposal pending review |
| `Edit...` | `Global` | *Edit...* | *Edit...* | Proposal pending review |
| `Change status` | `Global` | *Change status* | *Change status* | Proposal pending review |
| `Add interview...` | `Global` | *Add interview...* | *Add interview...* | Proposal pending review |
| `Open posting URL` | `Global` | *Open posting URL* | *Open posting URL* | Proposal pending review |
| `Duplicate` | `Global` | *Duplicate* | *Duplicate* | Proposal pending review |
| `Delete...` | `Global` | *Delete...* | *Delete...* | Proposal pending review |
| `Delete application` | `Global` | *Delete application* | *Delete application* | Proposal pending review |
| `Delete failed` | `Global` | *Delete failed* | *Delete failed* | Proposal pending review |
| `Status change failed` | `Global` | *Status change failed* | *Status change failed* | Proposal pending review |
| `Settings` | `GoalSpinBox` | Налаштування | Настройки | Navigation page |
| `Appearance` | `GoalSpinBox` | *Appearance* | *Appearance* | Proposal pending review |
| `System Default` | `GoalSpinBox` | *System Default* | *System Default* | Proposal pending review |
| `Light` | `GoalSpinBox` | *Light* | *Light* | Proposal pending review |
| `Dark` | `GoalSpinBox` | *Dark* | *Dark* | Proposal pending review |
| `Theme:` | `GoalSpinBox` | *Theme:* | *Theme:* | Proposal pending review |
| `Indigo` | `GoalSpinBox` | *Indigo* | *Indigo* | Proposal pending review |
| `Teal` | `GoalSpinBox` | *Teal* | *Teal* | Proposal pending review |
| `Rose` | `GoalSpinBox` | *Rose* | *Rose* | Proposal pending review |
| `Amber` | `GoalSpinBox` | *Amber* | *Amber* | Proposal pending review |
| `Emerald` | `GoalSpinBox` | *Emerald* | *Emerald* | Proposal pending review |
| `Sky` | `GoalSpinBox` | *Sky* | *Sky* | Proposal pending review |
| `Accent color:` | `GoalSpinBox` | *Accent color:* | *Accent color:* | Proposal pending review |
| `Select interface language` | `GoalSpinBox` | *Select interface language* | *Select interface language* | Proposal pending review |
| `Language:` | `GoalSpinBox` | *Language:* | *Language:* | Proposal pending review |
| `Preferences` | `GoalSpinBox` | *Preferences* | *Preferences* | Proposal pending review |
| `Weekly study goal:` | `GoalSpinBox` | *Weekly study goal:* | *Weekly study goal:* | Proposal pending review |
| `Monday` | `GoalSpinBox` | *Monday* | *Monday* | Proposal pending review |
| `Sunday` | `GoalSpinBox` | *Sunday* | *Sunday* | Proposal pending review |
| `15 minutes` | `GoalSpinBox` | *15 minutes* | *15 minutes* | Proposal pending review |
| `30 minutes` | `GoalSpinBox` | *30 minutes* | *30 minutes* | Proposal pending review |
| `1 hour` | `GoalSpinBox` | *1 hour* | *1 hour* | Proposal pending review |
| `2 hours` | `GoalSpinBox` | *2 hours* | *2 hours* | Proposal pending review |
| `1 day` | `GoalSpinBox` | *1 day* | *1 day* | Proposal pending review |
| `2 days` | `GoalSpinBox` | *2 days* | *2 days* | Proposal pending review |
| `Custom...` | `GoalSpinBox` | *Custom...* | *Custom...* | Proposal pending review |
| ` min` | `GoalSpinBox` | * min* | * min* | Proposal pending review |
| `Add lead` | `GoalSpinBox` | *Add lead* | *Add lead* | Proposal pending review |
| `Remove selected` | `GoalSpinBox` | *Remove selected* | *Remove selected* | Proposal pending review |
| `Reset to default` | `GoalSpinBox` | *Reset to default* | *Reset to default* | Proposal pending review |
| `Data & Backups` | `GoalSpinBox` | *Data  Backups* | *Data  Backups* | Proposal pending review |
| `Export Applications (CSV)` | `GoalSpinBox` | *Export Applications (CSV)* | *Export Applications (CSV)* | Proposal pending review |
| `Export Interviews (ICS)` | `GoalSpinBox` | *Export Interviews (ICS)* | *Export Interviews (ICS)* | Proposal pending review |
| `Backup Database (JSON)` | `GoalSpinBox` | *Backup Database (JSON)* | *Backup Database (JSON)* | Proposal pending review |
| `Restore Database...` | `GoalSpinBox` | *Restore Database...* | *Restore Database...* | Proposal pending review |
| `Load Sample Data` | `GoalSpinBox` | *Load Sample Data* | *Load Sample Data* | Proposal pending review |
| `Open Data Folder` | `GoalSpinBox` | *Open Data Folder* | *Open Data Folder* | Proposal pending review |
| `Study` | `Global` | Навчання | Обучение | Study page |
| `No Study Topics` | `Global` | *No Study Topics* | *No Study Topics* | Proposal pending review |
| `No Study Tracks` | `Global` | *No Study Tracks* | *No Study Tracks* | Proposal pending review |
| `+ Track` | `Global` | *+ Track* | *+ Track* | Proposal pending review |
| `Manage tracks...` | `Global` | *Manage tracks...* | *Manage tracks...* | Proposal pending review |
| `Board` | `Global` | *Board* | *Board* | Proposal pending review |
| `List` | `Global` | *List* | *List* | Proposal pending review |
| `Search topics (Ctrl+F)` | `Global` | *Search topics (Ctrl+F)* | *Search topics (Ctrl+F)* | Proposal pending review |
| `+ Topic` | `Global` | *+ Topic* | *+ Topic* | Proposal pending review |
| `Delete Topic` | `Global` | *Delete Topic* | *Delete Topic* | Proposal pending review |
| `Interview` | `AgendaKindStyle` | Співбесіда | Собеседование | Job interview |
| `Follow-up` | `AgendaKindStyle` | Фолоу-ап | Фоллоу-ап | Distinct from Reminder (Нагадування / Напоминание) |
| `Study target` | `AgendaKindStyle` | Навчальна ціль | Учебная цель | Agenda study milestone |
| `Wishlist` | `StatusStyle` | Список бажань | Список желаний | Application wishlist |
| `Applied` | `StatusStyle` | Подано | Подано | Hiring stage: application submitted |
| `HR Screen` | `StatusStyle` | HR-скринінг | HR-скрининг | Interview stage |
| `Technical` | `StatusStyle` | Технічне | Техническое | Interview type or stage |
| `Final` | `StatusStyle` | Фінальне | Финальное | Interview stage |
| `Offer` | `StatusStyle` | Офер | Оффер | Hiring stage |
| `Accepted` | `StatusStyle` | Прийнято | Принято | Hiring stage outcome |
| `Rejected` | `StatusStyle` | Відхилено | Отклонено | Hiring stage outcome |
| `Withdrawn` | `StatusStyle` | Відкликано | Отозвано | Application withdrawn |
| `Ghosted` | `StatusStyle` | Без відповіді | Без ответа | Application ghosted |
| `Backlog` | `StatusStyle` | Беклог | Бэклог | Topic board status |
| `In Progress` | `StatusStyle` | В процесі | В процессе | Topic board status |
| `Review` | `StatusStyle` | На перегляді | На проверке | Topic board status |
| `Done` | `StatusStyle` | Виконано | Выполнено | Topic board status |
| `Low` | `StatusStyle` | Низький | Низкий | Priority |
| `Normal` | `StatusStyle` | Звичайний | Обычный | Priority |
| `High` | `StatusStyle` | Високий | Высокий | Priority |
| `HR screen` | `StatusStyle` | HR-скринінг | HR-скрининг | Interview type |
| `Live coding` | `StatusStyle` | Лайвкодинг | Лайвкодинг | Interview type |
| `System design` | `StatusStyle` | Системний дизайн | Системный дизайн | Interview type |
| `Behavioral` | `StatusStyle` | Поведінкове | Поведенческое | Interview type |
| `Other` | `StatusStyle` | Інше | Другое | Category/type |
| `Pending` | `StatusStyle` | Очікується | Ожидается | Interview outcome |
| `Passed` | `StatusStyle` | Пройдено | Пройдено | Interview outcome |
| `Failed` | `StatusStyle` | Не пройдено | Не пройдено | Interview outcome |
| `Cancelled` | `StatusStyle` | Скасовано | Отменено | Interview outcome |
| `Interview rate: ` | `Global` | *Interview rate: * | *Interview rate: * | Proposal pending review |
| `Weekly study goal` | `Global` | *Weekly study goal* | *Weekly study goal* | Proposal pending review |
| `Weekly goal` | `Global` | *Weekly goal* | *Weekly goal* | Proposal pending review |
| `Study progress rings` | `Global` | *Study progress rings* | *Study progress rings* | Proposal pending review |
| `Move to` | `Global` | *Move to* | *Move to* | Proposal pending review |
| `Delete topic` | `Global` | *Delete topic* | *Delete topic* | Proposal pending review |
| `Topic details` | `InternalWriteGuard` | *Topic details* | *Topic details* | Proposal pending review |
| `Close details` | `InternalWriteGuard` | *Close details* | *Close details* | Proposal pending review |
| `Topic title` | `InternalWriteGuard` | *Topic title* | *Topic title* | Proposal pending review |
| `No target date` | `InternalWriteGuard` | *No target date* | *No target date* | Proposal pending review |
| `Clear` | `InternalWriteGuard` | *Clear* | *Clear* | Proposal pending review |
| `Tags, comma separated` | `InternalWriteGuard` | *Tags, comma separated* | *Tags, comma separated* | Proposal pending review |
| `Notes` | `InternalWriteGuard` | *Notes* | *Notes* | Proposal pending review |
| `Checklist` | `InternalWriteGuard` | *Checklist* | *Checklist* | Proposal pending review |
| `New checklist item` | `InternalWriteGuard` | *New checklist item* | *New checklist item* | Proposal pending review |
| `Add` | `InternalWriteGuard` | *Add* | *Add* | Proposal pending review |
| `Resources` | `InternalWriteGuard` | *Resources* | *Resources* | Proposal pending review |
| `Title` | `InternalWriteGuard` | *Title* | *Title* | Proposal pending review |
| `URL` | `InternalWriteGuard` | *URL* | *URL* | Proposal pending review |
| `Resource title` | `InternalWriteGuard` | *Resource title* | *Resource title* | Proposal pending review |
| `https://...` | `InternalWriteGuard` | *https://...* | *https://...* | Proposal pending review |
| `Open` | `InternalWriteGuard` | Відкрити | Открыть | Action button to open external link |
| `Delete` | `InternalWriteGuard` | Видалити | Удалить | Destructive action |
| `Log time` | `InternalWriteGuard` | *Log time* | *Log time* | Proposal pending review |
| `Select a topic` | `InternalWriteGuard` | *Select a topic* | *Select a topic* | Proposal pending review |
