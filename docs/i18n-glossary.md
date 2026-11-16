# JobPrep Translation Glossary & Terminology Proposals

> [!IMPORTANT]
> Section 1 rows have been **applied** in `jobprep_uk.ts` and `jobprep_ru.ts` in Part D.
> Open questions requiring user decision/sign-off are listed below.

## 1. Section 1: Needs the User's Decision (Domain Terminology)

This section contains domain-critical terms carrying specific semantic weight in JobPrep.
Each term requires the user's review and explicit sign-off prior to authoring full translations in Part D.

### Core Terminology Conflicts & Resolutions

1. **`Follow-up` is NOT `Reminder`:**
   - **`Follow-up`** is an action step or check-in date scheduled for a job application (**`Фолоу-ап`** in uk, **`Фоллоу-ап`** in ru).
   - **`Reminder`** refers to the background reminder service notification popping up in the system tray or banner (**`Нагадування`** in uk, **`Напоминание`** in ru).
2. **`Application` (Job application):**
   - Unified as **`Заявка`** in both Ukrainian and Russian per user specification (distinguished from an application program via `//:` comments).
3. **`Target`:**
   - Represents a date-bound study topic goal (**`Ціль`** in uk, **`Цель`** in ru).
4. **`Study session` / `session`:**
   - Unified as **`Заняття`** in uk (neuter; gen. pl. **`занять`**) and **`Занятие`** in ru. (*Reviewer proposal: changed in closeout, user may revert; in Ukrainian "сесія" primarily denotes an academic examination period*).
5. **`Discard`:**
   - Aligned with Qt's standard `QDialogButtonBox::Discard` translations from `qtbase_*.qm`: **`Відхилити`** in uk, **`Отклонить`** in ru.
6. **`Custom`:**
   - Unified as **`Власний`** in uk, **`Свой`** in ru across both `TrackDialog` and Settings.
7. **`Sample data`:**
   - Unified as **`Демонстраційні дані`** in uk, **`Демонстрационные данные`** in ru across titles and descriptions.

### Proposal on Accelerators / Mnemonics (`&`)

- **Context:** The English UI includes 16 accelerator mnemonics (`&Save`, `&Delete`, `&Application *`, `+ &Interview`, etc.) enabling `Alt+<Letter>` key combinations.
- **Qt Base Translations:** Converting `qtbase_uk.qm` with `lconvert` reveals that Qt's standard buttons (`QDialogButtonBox::Ok`, `Cancel`, `Yes`, `No`) already contain 280 Cyrillic accelerators (e.g. `Т&ак` for Alt+А, `&Ні` for Alt+Н, `С&касувати` for Alt+К).
- **Trade-off:**
  - If custom Cyrillic mnemonics are used, a user with an English keyboard layout active cannot trigger them without switching layouts, and there is a risk of duplicate accelerator collisions within complex custom dialogs.
  - If omitted in custom strings, the user loses `Alt+<Letter>` shortcuts for custom field labels, but standard dialog actions (`Enter`, `Esc`, standard buttons) continue working predictably.
- **User Decision Options:**
  - **Option A (Full Mnemonics):** Assign unique Cyrillic accelerator letters to dialog buttons and field labels (e.g. `&Зберегти`, `&Видалити`).
  - **Option B (Recommended — Minimal Risk):** Omit mnemonics in custom application strings; rely on standard `qtbase_*.qm` accelerators for dialog buttons and global shortcuts (`Ctrl+S`, `Ctrl+N`).

### Open Questions for User Sign-Off

1. **Accelerators / Mnemonics (`&`):**
   - **Applied (Option B):** Custom accelerators are omitted in Ukrainian and Russian catalogs to avoid layout collisions, untranslatable shortcuts on Latin layout, and shortcut conflicts. Standard dialog buttons (OK, Cancel, Yes, No) continue using Qt's built-in Cyrillic mnemonics via `qtbase_*.qm`.
   - **User Choice:** Confirm Option B or decide if Option A (full custom mnemonics) is required.
2. **User-Addressing Strings (Polite Plural):**
   - Per style guide, UI text avoids addressing the user (`ви/ти`, `вы/ты`) unless sentence clarity demands it. Four strings in Ukrainian and three in Russian use the polite lower-case plural (`вашої/вашей`, `ви/вы`). See `docs/reports/i18n-review.md` for the exact strings.
3. **Domain Terminology Sign-off:**
   - `Application`: Unified as **`Заявка`** in both Ukrainian and Russian.
   - `Follow-up`: Disambiguated as **`Фолоу-ап`** (uk) / **`Фоллоу-ап`** (ru), distinct from `Reminder` (**`Нагадування`** / **`Напоминание`**).
   - `Target`: Translated as **`Ціль`** (uk) / **`Цель`** (ru).
   - `Track`: Translated as **`Напрям`** (uk) / **`Направление`** (ru).
   - `Study streak`: Translated as **`Серія навчання`** (uk) / **`Серия занятий`** (ru).

### Domain Terms Table (40 Terms)

| English UI String | Context | Ukrainian Translation | Russian Translation | Status | Conflict / Semantic Notes |
| :--- | :--- | :--- | :--- | :---: | :--- |
| `Status` | `Global` | Статус | Статус | Applied | Disambiguated between hiring stage and study topic |
| `Applied` | `Global` | Подано | Подано | Applied | Hiring stage: application submitted |
| `Track` | `Global` | Напрям | Направление | Applied | Study track |
| `Priority` | `Global` | Пріоритет | Приоритет | Applied | Topic priority level |
| `Application` | `Global` | Заявка | Заявка | Applied | Unified term for job application |
| `Interview` | `Global` | Співбесіда | Собеседование | Applied | Job interview |
| `Topic` | `Global` | Тема | Тема | Applied | Study topic |
| `Interview` | `NotificationText` | Співбесіда | Собеседование | Applied | Job interview |
| `Status` | `TabScrollArea` | Статус | Статус | Applied | Disambiguated between hiring stage and study topic |
| `Work mode` | `TabScrollArea` | Формат роботи | Формат работы | Applied | Work mode (remote, onsite, hybrid) |
| `Today` | `Global` | Сьогодні | Сегодня | Applied | Current day |
| `Offer` | `Global` | Офер | Оффер | Applied | Hiring stage |
| `Interview` | `AgendaKindStyle` | Співбесіда | Собеседование | Applied | Job interview |
| `Follow-up` | `AgendaKindStyle` | Фолоу-ап | Фоллоу-ап | Applied | Distinct from Reminder (Нагадування / Напоминание) |
| `Wishlist` | `StatusStyle` | Список бажань | Список желаний | Applied | Application wishlist |
| `Applied` | `StatusStyle` | Подано | Подано | Applied | Hiring stage: application submitted |
| `HR Screen` | `StatusStyle` | HR-скринінг | HR-скрининг | Applied | Interview stage |
| `Technical` | `StatusStyle` | Технічне | Техническое | Applied | Interview type or stage |
| `Final` | `StatusStyle` | Фінальне | Финальное | Applied | Interview stage |
| `Offer` | `StatusStyle` | Офер | Оффер | Applied | Hiring stage |
| `Accepted` | `StatusStyle` | Прийнято | Принято | Applied | Hiring stage outcome |
| `Rejected` | `StatusStyle` | Відхилено | Отклонено | Applied | Hiring stage outcome |
| `Withdrawn` | `StatusStyle` | Відкликано | Отозвано | Applied | Application withdrawn |
| `Ghosted` | `StatusStyle` | Без відповіді | Без ответа | Applied | Application ghosted |
| `Backlog` | `StatusStyle` | Беклог | Бэклог | Applied | Topic board status |
| `In Progress` | `StatusStyle` | В процесі | В процессе | Applied | Topic board status |
| `Review` | `StatusStyle` | На перегляді | На проверке | Applied | Topic board status |
| `Done` | `StatusStyle` | Виконано | Выполнено | Applied | Topic board status |
| `HR screen` | `StatusStyle` | HR-скринінг | HR-скрининг | Applied | Interview type |
| `Live coding` | `StatusStyle` | Лайвкодинг | Лайвкодинг | Applied | Interview type |
| `System design` | `StatusStyle` | Системний дизайн | Системный дизайн | Applied | Interview type |
| `Behavioral` | `StatusStyle` | Поведінкове | Поведенческое | Applied | Interview type |
| `Pending` | `StatusStyle` | Очікується | Ожидается | Applied | Interview outcome |
| `Passed` | `StatusStyle` | Пройдено | Пройдено | Applied | Interview outcome |
| `Failed` | `StatusStyle` | Не пройдено | Не пройдено | Applied | Interview outcome |
| `Cancelled` | `StatusStyle` | Скасовано | Отменено | Applied | Interview outcome |
| `Checklist` | `InternalWriteGuard` | Чекліст | Чеклист | Applied | Study topic checklist |
| `Open` | `InternalWriteGuard` | Відкрити | Открыть | Applied | Action button to open external link |
| `Study target` | `AgendaKindStyle` | Навчальна ціль | Учебная цель | Applied | Date-bound study topic goal |
| `Study streak` | `Global` | Серія навчання | Серия занятий | Applied | Consecutive days of study progress |
| `Study session` | `Global` | Заняття | Занятие | Applied | Changed in closeout (user may revert); uk "сесія" means exam period |
| `Discard` | `Global` | Відхилити | Отклонить | Applied | Aligned with Qt standard QDialogButtonBox translation in qtbase |
| `Custom` | `Global` | Власний | Свой | Applied | Unified across TrackDialog and Settings |
| `Sample data` | `Global` | Демонстраційні дані | Демонстрационные данные | Applied | Unified across dialog titles and descriptions |

## 2. Section 2: Mechanical (Generic Labels, Buttons, Form Fields)

This section contains **281 mechanical UI terms** (standard button labels, table columns, settings options, and informational messages). These will be translated directly in Part D and only require spot-checking.

| English UI String | Context | Ukrainian Proposal | Russian Proposal | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `Load sample data?` | `Global` | *Load sample data?* | *Load sample data?* | Proposal pending review |
| `Sample data` | `Global` | *Sample data* | *Sample data* | Proposal pending review |
| `Company` | `Global` | Компанія | Компания | Application field |
| `Position` | `Global` | Посада | Должность | Application field |
| `Next step` | `Global` | *Next step* | *Next step* | Proposal pending review |
| `Salary` | `Global` | Зарплата | Зарплата | Application field |
| `Source` | `Global` | *Source* | *Source* | Proposal pending review |
| `Updated` | `Global` | *Updated* | *Updated* | Proposal pending review |
| `Title` | `Global` | *Title* | *Title* | Proposal pending review |
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
| `Source` | `TabScrollArea` | *Source* | *Source* | Proposal pending review |
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
| `System Default` | `SettingsPage` | Системна (за замовчуванням) | Системный (по умолчанию) | Applied | Language / theme selection |
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
| `Low` | `StatusStyle` | Низький | Низкий | Priority |
| `Normal` | `StatusStyle` | Звичайний | Обычный | Priority |
| `High` | `StatusStyle` | Високий | Высокий | Priority |
| `Other` | `StatusStyle` | Інше | Другое | Category/type |
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
| `New checklist item` | `InternalWriteGuard` | *New checklist item* | *New checklist item* | Proposal pending review |
| `Add` | `InternalWriteGuard` | *Add* | *Add* | Proposal pending review |
| `Resources` | `InternalWriteGuard` | *Resources* | *Resources* | Proposal pending review |
| `Title` | `InternalWriteGuard` | *Title* | *Title* | Proposal pending review |
| `URL` | `InternalWriteGuard` | *URL* | *URL* | Proposal pending review |
| `Resource title` | `InternalWriteGuard` | *Resource title* | *Resource title* | Proposal pending review |
| `https://...` | `InternalWriteGuard` | *https://...* | *https://...* | Proposal pending review |
| `Delete` | `InternalWriteGuard` | Видалити | Удалить | Destructive action |
| `Log time` | `InternalWriteGuard` | *Log time* | *Log time* | Proposal pending review |
| `Select a topic` | `InternalWriteGuard` | *Select a topic* | *Select a topic* | Proposal pending review |

## 3. Translator Comments (`//:`) in Codebase

Ten disambiguation comments were inserted into source code to ensure translators have complete context:

```
src/models/TopicListModel.cpp:209:            //: Header for study topic status column
src/models/ApplicationTableModel.cpp:168:            //: Header for job application stage column
src/models/ApplicationTableModel.cpp:171:            //: Header for date when job application was submitted
src/ui/widgets/TopicDetailPanel.cpp:233:    //: Button action to open link in external web browser
src/ui/MainWindow.cpp:379:    //: Menu action to create a new job application (not program)
src/ui/dialogs/ApplicationDialog.cpp:170:    //: Form field label for job application hiring stage
src/ui/pages/HomePage.cpp:151:    //: Dashboard card title showing today's agenda items
src/ui/pages/SettingsPage.cpp:269:    //: Notice displayed when user changes the interface language
src/ui/pages/CalendarPage.cpp:162:    //: Button action to navigate calendar view to current day
src/ui/theme/StatusStyle.cpp:39:            //: Application hiring stage: application has been submitted
```
