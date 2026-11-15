# JobPrep Translation Review: Sentences & Non-Trivial Strings

> [!NOTE]
> This document presents every non-trivial message (>3 words, placeholders `%1`..`%9`/`%n`, or plurals)
> for quick review by screen. Single words, standard labels, and status badges are documented in `docs/i18n-glossary.md`.

## Summary of Non-Trivial Strings by Screen

| Screen Section | String Count | Primary Source File(s) |
| :--- | :---: | :--- |
| **Home** | 10 | `src/ui/pages/HomePage.cpp`, `src/ui/widgets/GoalProgressBar.cpp`, `src/ui/widgets/TopicBoardColumn.cpp` |
| **Study** | 22 | `src/ui/delegates/TopicCardDelegate.cpp`, `src/ui/pages/StudyPage.cpp`, `src/ui/widgets/TopicDetailPanel.cpp` |
| **Jobs** | 8 | `src/ui/pages/JobsPage.cpp` |
| **Calendar** | 6 | `src/ui/pages/CalendarPage.cpp` |
| **Settings** | 61 | `src/ui/DataToolsController.cpp`, `src/ui/pages/SettingsPage.cpp` |
| **Dialogs** | 52 | `src/ui/dialogs/ApplicationDialog.cpp`, `src/ui/dialogs/FollowUpDialog.cpp`, `src/ui/dialogs/InterviewDialog.cpp`, `src/ui/dialogs/SessionDialog.cpp`, `src/ui/dialogs/TrackDialog.cpp` |
| **Notifications** | 7 | `src/ui/InAppNotifier.cpp`, `src/ui/NotificationText.cpp` |
| **General & Shell** | 22 | `src/main.cpp`, `src/ui/FormatHelper.cpp`, `src/ui/MainWindow.cpp`, `src/ui/widgets/SingleTabDateTime.h` |
| **Total** | **188** | |

## User-Addressing Strings (Polite Plural Form)

Per localization guidelines, UI text avoids directly addressing the user (`ви/ти`, `вы/ты`) unless sentence grammar demands it.
Where unavoidable, the polite plural form in lower case is employed (`вашої/вашей`, `ви/вы`):

| Context | English Source | Ukrainian Translation | Russian Translation | File |
| :--- | :--- | :--- | :--- | :--- |
| `DataToolsController` | Restoring from '%1' will REPLACE all current data.<br><br>A safety backup of your current database will be saved to:<br>%2<br><br>Do you want to continue? | Відновлення з «%1» ЗАМІНИТЬ усі поточні дані.<br><br>Захисну резервну копію поточної бази даних буде збережено до:<br>%2<br><br>Продовжити? | Восстановление из «%1» ЗАМЕНИТ все текущие данные.<br><br>Защитная резервная копия текущей базы данных будет сохранена в:<br>%2<br><br>Продолжить? | `src/ui/DataToolsController.cpp` |
| `Dialogs::FollowUpDialog` | What you will do on this day | Що заплановано зробити цього дня | Что запланировано сделать в этот день | `src/ui/dialogs/FollowUpDialog.cpp` |
| `Dialogs::InterviewDialog` | Who you will talk to | З ким відбудеться розмова | С кем будет разговор | `src/ui/dialogs/InterviewDialog.cpp` |
| `Pages::HomePage` | Welcome to JobPrep! Your daily study targets, active applications, and upcoming interviews will appear here. | Ласкаво просимо до JobPrep! Ваші щоденні навчальні цілі, активні заявки та майбутні співбесіди з'являтимуться тут. | Добро пожаловать в JobPrep! Ваши ежедневные учебные цели, активные заявки и предстоящие собеседования будут отображаться здесь. | `src/ui/pages/HomePage.cpp` |

## Home (10 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| Nothing planned for today | На сьогодні нічого не заплановано | На сегодня ничего не запланировано | `src/ui/pages/HomePage.cpp` |
| Log a study session or follow up on an application. | Запишіть заняття або виконайте фолоу-ап за заявкою. | Запишите занятие или выполните фоллоу-ап по заявке. | `src/ui/pages/HomePage.cpp` |
| Schedule your next interview stage or screening. | Заплануйте наступний етап співбесіди або скринінг. | Запланируйте следующий этап собеседования или скрининг. | `src/ui/pages/HomePage.cpp` |
| Welcome to JobPrep! Your daily study targets, active applications, and upcoming interviews will appear here. | Ласкаво просимо до JobPrep! Ваші щоденні навчальні цілі, активні заявки та майбутні співбесіди з'являтимуться тут. | Добро пожаловать в JobPrep! Ваши ежедневные учебные цели, активные заявки и предстоящие собеседования будут отображаться здесь. | `src/ui/pages/HomePage.cpp` |
| %1 of %2 (%3%) | %1 з %2 (%3%) | %1 из %2 (%3%) | `src/ui/widgets/GoalProgressBar.cpp` |
| Delete the topic and its checklist; logged study time is kept | Видалити тему та її чекліст; записаний час навчання зберігається | Удалить тему и её чеклист; записанное время занятий сохраняется | `src/ui/widgets/TopicBoardColumn.cpp` |
| Add a track with Manage tracks... to create topics | Додайте напрям через «Керування напрямами...», щоб створювати теми | Добавьте направление через «Управление направлениями...», чтобы создавать темы | `src/ui/widgets/TopicBoardColumn.cpp` |
| Topics need at least one track | Для тем потрібен щонайменше один напрям | Для тем требуется хотя бы одно направление | `src/ui/widgets/TopicBoardColumn.cpp` |
| New topic... (Adds to: %1) | Нова тема... (Додає до: %1) | Новая тема... (Добавляет в: %1) | `src/ui/widgets/TopicBoardColumn.cpp` |
| Press Enter to add a topic to this column (Adds to: %1) | Натисніть Enter, щоб додати тему до цієї колонки (Додає до: %1) | Нажмите Enter, чтобы добавить тему в эту колонку (Добавляет в: %1) | `src/ui/widgets/TopicBoardColumn.cpp` |

## Study (22 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| Priority: %1 | Пріоритет: %1 | Приоритет: %1 | `src/ui/delegates/TopicCardDelegate.cpp` |
| Organize your C++ and English interview topics into kanban columns and log study sessions. | Впорядковуйте теми співбесід із C++ та English у колонках канбану та записуйте заняття. | Упорядочивайте темы собеседований по C++ и English в колонках канбана и записывайте занятия. | `src/ui/pages/StudyPage.cpp` |
| No topic matches the current search and track filter. | Жодна тема не відповідає поточному пошуку та фільтру напряму. | Ни одна тема не соответствует текущему поиску и фильтру направления. | `src/ui/pages/StudyPage.cpp` |
| Create a study track (like C++ or English) to start planning topics. | Створіть навчальний напрям (наприклад, C++ або English), щоб розпочати планування тем. | Создайте учебное направление (например, C++ или English), чтобы начать планирование тем. | `src/ui/pages/StudyPage.cpp` |
| Add, rename, recolor or delete tracks | Додавайте, перейменовуйте, змінюйте колір або видаляйте напрями | Добавляйте, переименовывайте, меняйте цвет или удаляйте направления | `src/ui/pages/StudyPage.cpp` |
| Add a topic to Backlog (Ctrl+N) | Додати тему до беклогу (Ctrl+N) | Добавить тему в бэклог (Ctrl+N) | `src/ui/pages/StudyPage.cpp` |
| Could not move the topic. | Не вдалося перемістити тему. | Не удалось переместить тему. | `src/ui/pages/StudyPage.cpp` |
| Could not save the topic. | Не вдалося зберегти тему. | Не удалось сохранить тему. | `src/ui/pages/StudyPage.cpp` |
| Delete “%1”? Its checklist goes away; logged study time is kept. | Видалити «%1»? Її чекліст буде видалено; записаний час навчання зберігається. | Удалить «%1»? Её чеклист будет удалён; записанное время занятий сохраняется. | `src/ui/pages/StudyPage.cpp` |
| Could not delete the topic. | Не вдалося видалити тему. | Не удалось удалить тему. | `src/ui/pages/StudyPage.cpp` |
| Could not reorder the topic. | Не вдалося змінити порядок тем. | Не удалось изменить порядок тем. | `src/ui/pages/StudyPage.cpp` |
| Could not add the topic. | Не вдалося додати тему. | Не удалось добавить тему. | `src/ui/pages/StudyPage.cpp` |
| Close the details (Esc) | Закрити деталі (Esc) | Закрыть детали (Esc) | `src/ui/widgets/TopicDetailPanel.cpp` |
| Track this topic belongs to | Напрям, до якого належить ця тема | Направление, к которому относится эта тема | `src/ui/widgets/TopicDetailPanel.cpp` |
| Clear the target date | Очистити цільову дату | Очистить целевую дату | `src/ui/widgets/TopicDetailPanel.cpp` |
| Only http and https links can be opened | Можна відкривати лише посилання http та https | Можно открывать только ссылки http и https | `src/ui/widgets/TopicDetailPanel.cpp` |
| Log study session for this topic | Записати заняття для цієї теми | Записать занятие для этой темы | `src/ui/widgets/TopicDetailPanel.cpp` |
| Title cannot be empty. | Назва не може бути порожньою. | Название не может быть пустым. | `src/ui/widgets/TopicDetailPanel.cpp` |
| Tracks changed. Please reselect. | Напрями змінилися. Будь ласка, виберіть знову. | Направления изменились. Пожалуйста, выберите снова. | `src/ui/widgets/TopicDetailPanel.cpp` |
| Checklist text cannot be empty. | Текст чекліста не може бути порожнім. | Текст чеклиста не может быть пустым. | `src/ui/widgets/TopicDetailPanel.cpp` |
| Only http and https links can be added. | Можна додавати лише посилання http та https. | Можно добавлять только ссылки http и https. | `src/ui/widgets/TopicDetailPanel.cpp` |
| Only http and https links can be opened. | Можна відкривати лише посилання http та https. | Можно открывать только ссылки http и https. | `src/ui/widgets/TopicDetailPanel.cpp` |

## Jobs (8 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| Search company, position, notes | Пошук за компанією, посадою, нотатками | Поиск по компании, должности, заметкам | `src/ui/pages/JobsPage.cpp` |
| Export the rows you can see to a CSV file | Експортувати видимі рядки у файл CSV | Экспортировать видимые строки в файл CSV | `src/ui/pages/JobsPage.cpp` |
| Add a job application (Ctrl+N) | Додати заявку на роботу (Ctrl+N) | Добавить заявку на работу (Ctrl+N) | `src/ui/pages/JobsPage.cpp` |
| Track job openings, manage interview stages, and follow up with recruiters. | Відстежуйте вакансії, керуйте етапами співбесід і підтримуйте контакт із рекрутерами. | Отслеживайте вакансии, управляйте этапами собеседований и поддерживайте контакт с рекрутерами. | `src/ui/pages/JobsPage.cpp` |
| No application matches the current search and status filter. | Жодна заявка не відповідає поточному пошуку та фільтру статусу. | Ни одна заявка не соответствует текущему поиску и фильтру статуса. | `src/ui/pages/JobsPage.cpp` |
| Only http and https links can be opened | Можна відкривати лише посилання http та https | Можно открывать только ссылки http и https | `src/ui/pages/JobsPage.cpp` |
| %n application(s) | %n заявка / %n заявки / %n заявок | %n заявка / %n заявки / %n заявок | `src/ui/pages/JobsPage.cpp` |
| Delete the application at %1? Its interviews and timeline go too. | Видалити заявку в %1? Її співбесіди та хронологію також буде видалено. | Удалить заявку в %1? Её собеседования и хронология также будут удалены. | `src/ui/pages/JobsPage.cpp` |

## Calendar (6 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| Previous month (Page Up) | Попередній місяць (Page Up) | Предыдущий месяц (Page Up) | `src/ui/pages/CalendarPage.cpp` |
| Next month (Page Down) | Наступний місяць (Page Down) | Следующий месяц (Page Down) | `src/ui/pages/CalendarPage.cpp` |
| Plan a follow-up action on the selected day | Запланувати фолоу-ап на вибраний день | Запланировать фоллоу-ап на выбранный день | `src/ui/pages/CalendarPage.cpp` |
| Plan and manage study target dates on the Study page | Плануйте навчальні цілі та керуйте ними на сторінці «Навчання» | Планируйте учебные цели и управляйте ими на странице «Обучение» | `src/ui/pages/CalendarPage.cpp` |
| Use + Interview or + Follow-up to schedule something here. | Скористайтеся кнопками «+ Співбесіда» або «+ Фолоу-ап», щоб запланувати подію. | Используйте «+ Собеседование» или «+ Фоллоу-ап», чтобы запланировать событие. | `src/ui/pages/CalendarPage.cpp` |
| Select a date to view or schedule events. | Виберіть дату для перегляду або планування подій. | Выберите дату для просмотра или планирования событий. | `src/ui/pages/CalendarPage.cpp` |

## Settings (61 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| %1 h | %1 год | %1 ч | `src/ui/pages/SettingsPage.cpp` |
| %1 h %2 min | %1 год %2 хв | %1 ч %2 мин | `src/ui/pages/SettingsPage.cpp` |
| Could not open the selected backup file. | Не вдалося відкрити вибраний файл резервної копії. | Не удалось открыть выбранный файл резервной копии. | `src/ui/DataToolsController.cpp` |
| The backup file exceeds the maximum allowed size (64 MB). | Файл резервної копії перевищує максимально дозволений розмір (64 МБ). | Файл резервной копии превышает максимально допустимый размер (64 МБ). | `src/ui/DataToolsController.cpp` |
| The backup file is empty. | Файл резервної копії порожній. | Файл резервной копии пуст. | `src/ui/DataToolsController.cpp` |
| The file is not a valid JSON document. | Файл не є дійсним документом JSON. | Файл не является корректным документом JSON. | `src/ui/DataToolsController.cpp` |
| The file is not a valid JobPrep backup (format mismatch). | Файл не є дійсною резервною копією JobPrep (невідповідність формату). | Файл не является корректной резервной копией JobPrep (несоответствие формата). | `src/ui/DataToolsController.cpp` |
| The backup version is not supported by this version of JobPrep. | Версія резервної копії не підтримується цією версією JobPrep. | Версия резервной копии не поддерживается этой версией JobPrep. | `src/ui/DataToolsController.cpp` |
| The backup file contains duplicate item identifiers. | Файл резервної копії містить повторювані ідентифікатори елементів. | Файл резервной копии содержит повторяющиеся идентификаторы элементов. | `src/ui/DataToolsController.cpp` |
| The backup file contains invalid non-positive identifiers. | Файл резервної копії містить неприпустимі ідентифікатори (нуль або від'ємні значення). | Файл резервной копии содержит недопустимые идентификаторы (ноль или отрицательные значения). | `src/ui/DataToolsController.cpp` |
| The backup file contains unrecognized status or priority values. | Файл резервної копії містить нерозпізнані значення статусу або пріоритету. | Файл резервной копии содержит нераспознанные значения статуса или приоритета. | `src/ui/DataToolsController.cpp` |
| The backup file contains invalid date or time format strings. | Файл резервної копії містить некоректні рядки формату дати або часу. | Файл резервной копии содержит некорректные строки формата даты или времени. | `src/ui/DataToolsController.cpp` |
| The backup file contains broken cross-table references. | Файл резервної копії містить пошкоджені міжтабличні посилання. | Файл резервной копии содержит повреждённые межтабличные ссылки. | `src/ui/DataToolsController.cpp` |
| The backup file is missing required fields or table entries. | У файлі резервної копії відсутні обов'язкові поля або записи таблиць. | В файле резервной копии отсутствуют обязательные поля или записи таблиц. | `src/ui/DataToolsController.cpp` |
| Could not create the safety pre-restore backup. The restore was aborted. | Не вдалося створити захисну резервну копію перед відновленням. Відновлення перервано. | Не удалось создать защитную резервную копию перед восстановлением. Восстановление прервано. | `src/ui/DataToolsController.cpp` |
| A database error occurred while replacing tables. | Під час заміни таблиць сталася помилка бази даних. | При замене таблиц произошла ошибка базы данных. | `src/ui/DataToolsController.cpp` |
| An unknown restore error occurred. | Сталася невідома помилка відновлення. | Произошла неизвестная ошибка восстановления. | `src/ui/DataToolsController.cpp` |
| CSV Files (*.csv);;All Files (*) | Файли CSV (*.csv);;Усі файли (*) | Файлы CSV (*.csv);;Все файлы (*) | `src/ui/DataToolsController.cpp` |
| Applications successfully exported to:<br>%1 | Заявки успішно експортовано до:<br>%1 | Заявки успешно экспортированы в:<br>%1 | `src/ui/DataToolsController.cpp` |
| Could not export applications:<br>%1 | Не вдалося експортувати заявки:<br>%1 | Не удалось экспортировать заявки:<br>%1 | `src/ui/DataToolsController.cpp` |
| iCalendar Files (*.ics);;All Files (*) | Файли iCalendar (*.ics);;Усі файли (*) | Файлы iCalendar (*.ics);;Все файлы (*) | `src/ui/DataToolsController.cpp` |
| Interviews successfully exported to:<br>%1 | Співбесіди успішно експортовано до:<br>%1 | Собеседования успешно экспортированы в:<br>%1 | `src/ui/DataToolsController.cpp` |
| Could not export interviews:<br>%1 | Не вдалося експортувати співбесіди:<br>%1 | Не удалось экспортировать собеседования:<br>%1 | `src/ui/DataToolsController.cpp` |
| JSON Backup Files (*.json);;All Files (*) | Файли резервних копій JSON (*.json);;Усі файли (*) | Файлы резервных копий JSON (*.json);;Все файлы (*) | `src/ui/DataToolsController.cpp` |
| Database backup successfully created at:<br>%1 | Резервну копію бази даних успішно створено за адресою:<br>%1 | Резервная копия базы данных успешно создана в:<br>%1 | `src/ui/DataToolsController.cpp` |
| Could not create database backup:<br>%1 | Не вдалося створити резервну копію бази даних:<br>%1 | Не удалось создать резервную копию базы данных:<br>%1 | `src/ui/DataToolsController.cpp` |
| A database restore is already in progress. | Відновлення бази даних вже виконується. | Восстановление базы данных уже выполняется. | `src/ui/DataToolsController.cpp` |
| Restore Database from Backup | Відновити базу даних із резервної копії | Восстановить базу данных из резервной копии | `src/ui/DataToolsController.cpp` |
| Restoring from '%1' will REPLACE all current data.<br><br>A safety backup of your current database will be saved to:<br>%2<br><br>Do you want to continue? | Відновлення з «%1» ЗАМІНИТЬ усі поточні дані.<br><br>Захисну резервну копію поточної бази даних буде збережено до:<br>%2<br><br>Продовжити? | Восстановление из «%1» ЗАМЕНИТ все текущие данные.<br><br>Защитная резервная копия текущей базы данных будет сохранена в:<br>%2<br><br>Продолжить? | `src/ui/DataToolsController.cpp` |
| Restoring database from backup file. Please wait... | Відновлення бази даних із файлу резервної копії. Зачекайте... | Восстановление базы данных из файла резервной копии. Подождите... | `src/ui/DataToolsController.cpp` |
| Database restore completed successfully.<br><br>Restored records:<br>- %1 tracks<br>- %2 topics<br>- %3 subtasks<br>- %4 study sessions<br>- %5 applications<br>- %6 status history entries<br>- %7 interviews<br><br>Safety backup saved to:<br>%8 | Відновлення бази даних успішно завершено.<br><br>Відновлені записи:<br>- Напрями: %1<br>- Теми: %2<br>- Підзавдання: %3<br>- Заняття: %4<br>- Заявки: %5<br>- Записи історії статусів: %6<br>- Співбесіди: %7<br><br>Захисну резервну копію збережено до:<br>%8 | Восстановление базы данных успешно завершено.<br><br>Восстановленные записи:<br>- Направления: %1<br>- Темы: %2<br>- Подзадачи: %3<br>- Занятия: %4<br>- Заявки: %5<br>- Записи истории статусов: %6<br>- Собеседования: %7<br><br>Защитная резервная копия сохранена в:<br>%8 | `src/ui/DataToolsController.cpp` |
| Could not restore database:<br>%1%2<br><br>Your current database was NOT modified. | Не вдалося відновити базу даних:<br>%1%2<br><br>Поточну базу даних НЕ змінено. | Не удалось восстановить базу данных:<br>%1%2<br><br>Текущая база данных НЕ изменена. | `src/ui/DataToolsController.cpp` |
| Your database already contains data. Loading sample data will add standard study tracks and topics without deleting your existing items.<br><br>Do you want to continue? | База даних вже містить дані. Завантаження демонстраційних даних додасть стандартні навчальні напрями та теми, не видаляючи наявні елементи.<br><br>Продовжити? | База данных уже содержит данные. Загрузка демонстрационных данных добавит стандартные направления и темы, не удаляя существующие элементы.<br><br>Продолжить? | `src/ui/DataToolsController.cpp` |
| Confirm Load Sample Data | Підтвердження завантаження демонстраційних даних | Подтверждение загрузки демонстрационных данных | `src/ui/DataToolsController.cpp` |
| Sample study tracks and topics were successfully loaded. | Демонстраційні навчальні напрями та теми успішно завантажено. | Демонстрационные направления и темы успешно загружены. | `src/ui/DataToolsController.cpp` |
| Load Sample Data Failed | Помилка завантаження демонстраційних даних | Ошибка загрузки демонстрационных данных | `src/ui/DataToolsController.cpp` |
| Could not load sample data: %1 | Не вдалося завантажити демонстраційні дані: %1 | Не удалось загрузить демонстрационные данные: %1 | `src/ui/DataToolsController.cpp` |
| Could not open the data folder in file manager.<br><br>Path: %1 | Не вдалося відкрити папку даних у файловому менеджері.<br><br>Шлях: %1 | Не удалось открыть папку данных в файловом менеджере.<br><br>Путь: %1 | `src/ui/DataToolsController.cpp` |
| Keep JobPrep running in the system tray when closing the window | Залишати JobPrep працювати в системному треї під час закриття вікна | Оставлять JobPrep работать в системном трее при закрытии окна | `src/ui/pages/SettingsPage.cpp` |
| System tray is not available on this system | Системний трей недоступний у цій системі | Системный трей недоступен в этой системе | `src/ui/pages/SettingsPage.cpp` |
| Select visual color theme | Виберіть візуальну кольорову тему | Выберите визуальную цветовую тему | `src/ui/pages/SettingsPage.cpp` |
| Select accent highlight color | Виберіть акцентний колір підсвічування | Выберите акцентный цвет выделения | `src/ui/pages/SettingsPage.cpp` |
| Language will take effect after restarting JobPrep. | Мову буде змінено після перезапуску JobPrep. | Язык изменится после перезапуска JobPrep. | `src/ui/pages/SettingsPage.cpp` |
| Weekly study goal in hours and minutes (e.g. 8, 7:30, 7;30, 450 min; bare numbers or decimals mean hours) | Щотижнева навчальна ціль у годинах і хвилинах (наприклад: 8, 7:30, 7;30, 450 min; числа чи десяткові дроби означають години) | Еженедельная учебная цель в часах и минутах (например: 8, 7:30, 7;30, 450 min; числа или десятичные дроби означают часы) | `src/ui/pages/SettingsPage.cpp` |
| First day of the week for calendar and study statistics | Перший день тижня для календаря та навчальної статистики | Первый день недели для календаря и учебной статистики | `src/ui/pages/SettingsPage.cpp` |
| First day of week: | Перший день тижня: | Первый день недели: | `src/ui/pages/SettingsPage.cpp` |
| Minimize / Close to system tray | Згортати / закривати в системний трей | Сворачивать / закрывать в системный трей | `src/ui/pages/SettingsPage.cpp` |
| Interview Reminder Lead Times | Час завчасного нагадування про співбесіди | Время заблаговременного напоминания о собеседованиях | `src/ui/pages/SettingsPage.cpp` |
| Active interview reminder notifications before start time | Активні сповіщення-нагадування про співбесіду до часу початку | Активные уведомления-напоминания о собеседовании до времени начала | `src/ui/pages/SettingsPage.cpp` |
| Select a preset lead time or choose Custom | Виберіть готовий інтервал або вкажіть власний | Выберите готовый интервал или укажите свой | `src/ui/pages/SettingsPage.cpp` |
| Custom reminder lead time in minutes (1 - 10080) | Власний інтервал нагадування у хвилинах (1 - 10080) | Свой интервал напоминания в минутах (1 - 10080) | `src/ui/pages/SettingsPage.cpp` |
| Add reminder lead time to the list | Додати інтервал нагадування до списку | Добавить интервал напоминания в список | `src/ui/pages/SettingsPage.cpp` |
| Remove the selected reminder lead time | Видалити вибраний інтервал нагадування | Удалить выбранный интервал напоминания | `src/ui/pages/SettingsPage.cpp` |
| Reset reminder lead times to defaults (1 day and 1 hour) | Скинути інтервали нагадування до стандартних (1 день і 1 година) | Сбросить интервалы напоминания на стандартные (1 день и 1 час) | `src/ui/pages/SettingsPage.cpp` |
| Export applications and interviews, create full JSON backups, or load sample preparation tracks. | Експортуйте заявки та співбесіди, створюйте повні резервні копії JSON або завантажуйте демонстраційні напрями підготовки. | Экспортируйте заявки и собеседования, создавайте полные резервные копии JSON или загружайте демонстрационные направления подготовки. | `src/ui/pages/SettingsPage.cpp` |
| Export job applications as a CSV spreadsheet | Експортувати заявки на роботу у вигляді таблиці CSV | Экспортировать заявки на работу в виде таблицы CSV | `src/ui/pages/SettingsPage.cpp` |
| Export upcoming and past interviews as an iCalendar file | Експортувати майбутні та минулі співбесіди у файл iCalendar | Экспортировать предстоящие и прошлые собеседования в файл iCalendar | `src/ui/pages/SettingsPage.cpp` |
| Create a complete JSON backup of all tracks, topics, applications, and interviews | Створити повну резервну копію JSON для всіх напрямів, тем, заявок та співбесід | Создать полную резервную копию JSON для всех направлений, тем, заявок и собеседований | `src/ui/pages/SettingsPage.cpp` |
| Restore database from a JSON backup file (replaces current data) | Відновити базу даних із файлу резервної копії JSON (замінює поточні дані) | Восстановить базу данных из файла резервной копии JSON (заменяет текущие данные) | `src/ui/pages/SettingsPage.cpp` |
| Load standard interview preparation tracks and topics | Завантажити стандартні напрями та теми підготовки до співбесід | Загрузить стандартные направления и темы подготовки к собеседованиям | `src/ui/pages/SettingsPage.cpp` |
| Open the local folder containing application data and SQLite database | Відкрити локальну папку, що містить дані програми та базу даних SQLite | Открыть локальную папку, содержащую данные программы и базу данных SQLite | `src/ui/pages/SettingsPage.cpp` |

## Dialogs (52 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| Follow up with the recruiter | Зв'язатися з рекрутером | Связаться с рекрутером | `src/ui/dialogs/ApplicationDialog.cpp` |
| Recruiter feedback, interview topics, links... | Відгук рекрутера, теми співбесіди, посилання... | Отзыв рекрутера, темы собеседования, ссылки... | `src/ui/dialogs/ApplicationDialog.cpp` |
| No interviews scheduled yet | Співбесід ще не заплановано | Собеседований пока не запланировано | `src/ui/dialogs/ApplicationDialog.cpp` |
| Keep notes and outcomes for every interview stage. | Зберігайте нотатки та результати для кожного етапу співбесіди. | Сохраняйте заметки и результаты для каждого этапа собеседования. | `src/ui/dialogs/ApplicationDialog.cpp` |
| No status changes recorded | Змін статусу не зафіксовано | Изменений статуса не зафиксировано | `src/ui/dialogs/ApplicationDialog.cpp` |
| Status transitions will appear here as you update the application. | Зміни статусу з'являтимуться тут під час оновлення заявки. | Изменения статуса будут появляться здесь по мере обновления заявки. | `src/ui/dialogs/ApplicationDialog.cpp` |
| Interview changes are saved immediately and are not part of the Overview "Discard changes?" prompt. | Зміни співбесід зберігаються негайно і не входять до запиту «Відхилити зміни?» на вкладці «Огляд». | Изменения собеседований сохраняются сразу и не входят в запрос «Отклонить изменения?» на вкладке «Обзор». | `src/ui/dialogs/ApplicationDialog.cpp` |
| Save the application first to add interviews. | Спочатку збережіть заявку, щоб додавати співбесіди. | Сначала сохраните заявку, чтобы добавлять собеседования. | `src/ui/dialogs/ApplicationDialog.cpp` |
| %n min | %n хв / %n хв / %n хв | %n мин / %n мин / %n мин | `src/ui/dialogs/ApplicationDialog.cpp` |
| Add an interview to this application | Додати співбесіду до цієї заявки | Добавить собеседование к этой заявке | `src/ui/dialogs/ApplicationDialog.cpp` |
| Save the application first to add interviews | Спочатку збережіть заявку, щоб додавати співбесіди | Сначала сохраните заявку, чтобы добавлять собеседования | `src/ui/dialogs/ApplicationDialog.cpp` |
| Edit the selected interview | Редагувати вибрану співбесіду | Редактировать выбранное собеседование | `src/ui/dialogs/ApplicationDialog.cpp` |
| Select an interview to edit it | Виберіть співбесіду для редагування | Выберите собеседование для редактирования | `src/ui/dialogs/ApplicationDialog.cpp` |
| Delete the selected interview | Видалити вибрану співбесіду | Удалить выбранное собеседование | `src/ui/dialogs/ApplicationDialog.cpp` |
| Select an interview to delete it | Виберіть співбесіду для видалення | Выберите собеседование для удаления | `src/ui/dialogs/ApplicationDialog.cpp` |
| Delete this interview? This cannot be undone. | Видалити цю співбесіду? Цю дію не можна скасувати. | Удалить это собеседование? Это действие нельзя отменить. | `src/ui/dialogs/ApplicationDialog.cpp` |
| Could not delete: %1 | Не вдалося видалити: %1 | Не удалось удалить: %1 | `src/ui/dialogs/ApplicationDialog.cpp` |
| Salary min cannot be greater than salary max. | Мінімальна зарплата не може бути більшою за максимальну. | Минимальная зарплата не может быть больше максимальной. | `src/ui/dialogs/ApplicationDialog.cpp` |
| Warning: the contact e-mail does not look like an address. | Попередження: контактний email не схожий на дійсну адресу. | Предупреждение: контактный email не похож на корректный адрес. | `src/ui/dialogs/ApplicationDialog.cpp` |
| This application has unsaved changes. Discard them? | Ця заявка має незбережені зміни. Відхилити їх? | В этой заявке есть несохранённые изменения. Отклонить их? | `src/ui/dialogs/ApplicationDialog.cpp` |
| Could not save: %1 | Не вдалося зберегти: %1 | Не удалось сохранить: %1 | `src/ui/dialogs/ApplicationDialog.cpp` |
| No applications available. Add an application first. | Немає доступних заявок. Спочатку додайте заявку. | Нет доступных заявок. Сначала добавьте заявку. | `src/ui/dialogs/FollowUpDialog.cpp` |
| Describe the follow-up action. | Опишіть дію фолоу-апу. | Опишите действие фоллоу-апа. | `src/ui/dialogs/FollowUpDialog.cpp` |
| The application to follow up on | Заявка для фолоу-апу | Заявка для фоллоу-апа | `src/ui/dialogs/FollowUpDialog.cpp` |
| Remind the recruiter, send a thank-you note... | Нагадати рекрутеру, надіслати подяку... | Напомнить рекрутеру, отправить благодарность... | `src/ui/dialogs/FollowUpDialog.cpp` |
| What you will do on this day | Що заплановано зробити цього дня | Что запланировано сделать в этот день | `src/ui/dialogs/FollowUpDialog.cpp` |
| Planned date, taken from the selected day | Запланована дата, вибрана в календарі | Запланированная дата, выбранная в календаре | `src/ui/dialogs/FollowUpDialog.cpp` |
| Schedule the follow-up (Ctrl+S) | Запланувати фолоу-ап (Ctrl+S) | Запланировать фоллоу-ап (Ctrl+S) | `src/ui/dialogs/FollowUpDialog.cpp` |
| %1 already has a follow-up planned for %2. Replace it with %3? | Для %1 вже заплановано фолоу-ап на %2. Замінити його на %3? | Для %1 уже запланирован фоллоу-ап на %2. Заменить его на %3? | `src/ui/dialogs/FollowUpDialog.cpp` |
| The application this interview belongs to | Заявка, до якої належить ця співбесіда | Заявка, к которой относится это собеседование | `src/ui/dialogs/InterviewDialog.cpp` |
| Expected duration in minutes | Очікувана тривалість у хвилинах | Ожидаемая длительность в минутах | `src/ui/dialogs/InterviewDialog.cpp` |
| Which round this is | Який це раунд | Какой это раунд | `src/ui/dialogs/InterviewDialog.cpp` |
| Where the interview takes place | Де проходить співбесіда | Где проходит собеседование | `src/ui/dialogs/InterviewDialog.cpp` |
| Who you will talk to | З ким відбудеться розмова | С кем будет разговор | `src/ui/dialogs/InterviewDialog.cpp` |
| Preparation notes, questions to ask... | Нотатки для підготовки, запитання до інтерв'юера... | Заметки для подготовки, вопросы к интервьюеру... | `src/ui/dialogs/InterviewDialog.cpp` |
| Save the interview (Ctrl+S) | Зберегти співбесіду (Ctrl+S) | Сохранить собеседование (Ctrl+S) | `src/ui/dialogs/InterviewDialog.cpp` |
| An application is required. | Необхідно вказати заявку. | Необходимо указать заявку. | `src/ui/dialogs/InterviewDialog.cpp` |
| Duration must be at least 1 minute. | Тривалість має бути не менше 1 хвилини. | Длительность должна быть не менее 1 минуты. | `src/ui/dialogs/InterviewDialog.cpp` |
| This interview has unsaved changes. Discard them? | Ця співбесіда має незбережені зміни. Відхилити їх? | В этом собеседовании есть несохранённые изменения. Отклонить их? | `src/ui/dialogs/InterviewDialog.cpp` |
| Could not save: %1 | Не вдалося зберегти: %1 | Не удалось сохранить: %1 | `src/ui/dialogs/InterviewDialog.cpp` |
| Delete this interview? This cannot be undone. | Видалити цю співбесіду? Цю дію не можна скасувати. | Удалить это собеседование? Это действие нельзя отменить. | `src/ui/dialogs/InterviewDialog.cpp` |
| Could not delete: %1 | Не вдалося видалити: %1 | Не удалось удалить: %1 | `src/ui/dialogs/InterviewDialog.cpp` |
| Duration must be at least 1 minute. | Тривалість має бути не менше 1 хвилини. | Длительность должна быть не менее 1 минуты. | `src/ui/dialogs/SessionDialog.cpp` |
| Could not log session: %1 | Не вдалося записати заняття: %1 | Не удалось записать занятие: %1 | `src/ui/dialogs/SessionDialog.cpp` |
| Track name (e.g. C++, English) | Назва напряму (наприклад, C++, English) | Название направления (например, C++, English) | `src/ui/dialogs/TrackDialog.cpp` |
| Custom (%1) | Власний (%1) | Свой (%1) | `src/ui/dialogs/TrackDialog.cpp` |
| Track name cannot be empty. | Назва напряму не може бути порожньою. | Название направления не может быть пустым. | `src/ui/dialogs/TrackDialog.cpp` |
| A track with this name already exists. | Напрям із такою назвою вже існує. | Направление с таким названием уже существует. | `src/ui/dialogs/TrackDialog.cpp` |
| Could not update track: %1 | Не вдалося оновити напрям: %1 | Не удалось обновить направление: %1 | `src/ui/dialogs/TrackDialog.cpp` |
| Could not add track: %1 | Не вдалося додати напрям: %1 | Не удалось добавить направление: %1 | `src/ui/dialogs/TrackDialog.cpp` |
| Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept. | Видалити напрям «%1»? Це видалить %n тему та її чекліст. Записаний час навчання буде збережено. / Видалити напрям «%1»? Це видалить %n теми та їхні чеклісти. Записаний час навчання буде збережено. / Видалити напрям «%1»? Це видалить %n тем та їхні чеклісти. Записаний час навчання буде збережено. | Удалить направление «%1»? Это удалит %n тему и её чеклист. Записанное время занятий сохранится. / Удалить направление «%1»? Это удалит %n темы и их чеклисты. Записанное время занятий сохранится. / Удалить направление «%1»? Это удалит %n тем и их чеклисты. Записанное время занятий сохранится. | `src/ui/dialogs/TrackDialog.cpp` |
| Could not delete track: %1 | Не вдалося видалити напрям: %1 | Не удалось удалить направление: %1 | `src/ui/dialogs/TrackDialog.cpp` |

## Notifications (7 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| +%n more | +ще %n / +ще %n / +ще %n | +ещё %n / +ещё %n / +ещё %n | `src/ui/InAppNotifier.cpp` |
| in %n hour(s) | через %n годину / через %n години / через %n годин | через %n час / через %n часа / через %n часов | `src/ui/NotificationText.cpp` |
| in %n minute(s) | через %n хвилину / через %n хвилини / через %n хвилин | через %n минуту / через %n минуты / через %n минут | `src/ui/NotificationText.cpp` |
| today at %1 | сьогодні о %1 | сегодня в %1 | `src/ui/NotificationText.cpp` |
| tomorrow at %1 | завтра о %1 | завтра в %1 | `src/ui/NotificationText.cpp` |
| Interview %1 | Співбесіда %1 | Собеседование %1 | `src/ui/NotificationText.cpp` |
| %n follow-up(s) overdue | %n прострочений фолоу-ап / %n прострочені фолоу-апи / %n прострочених фолоу-апів | %n просроченный фоллоу-ап / %n просроченных фоллоу-апа / %n просроченных фоллоу-апов | `src/ui/NotificationText.cpp` |

## General & Shell (22 strings)

| English Source | Ukrainian Translation | Russian Translation | Source File |
| :--- | :--- | :--- | :--- |
| %1h %2m | %1год %2хв | %1ч %2мин | `src/ui/FormatHelper.cpp` |
| %1h | %1год | %1ч | `src/ui/FormatHelper.cpp` |
| %1m | %1хв | %1мин | `src/ui/FormatHelper.cpp` |
| %1 / %2 | %1 / %2 | %1 / %2 | `src/ui/FormatHelper.cpp` |
| %n day(s) | %n день / %n дні / %n днів | %n день / %n дня / %n дней | `src/ui/FormatHelper.cpp` |
| %n hour(s) | %n година / %n години / %n годин | %n час / %n часа / %n часов | `src/ui/FormatHelper.cpp` |
| %n minute(s) | %n хвилина / %n хвилини / %n хвилин | %n минута / %n минуты / %n минут | `src/ui/FormatHelper.cpp` |
| %n-day streak | %n-денна серія / %n-денна серія / %n-денна серія | %n-дневная серия / %n-дневная серия / %n-дневная серия | `src/ui/FormatHelper.cpp` |
| %n active process(es) | %n активний процес / %n активні процеси / %n активних процесів | %n активный процесс / %n активных процесса / %n активных процессов | `src/ui/FormatHelper.cpp` |
| %1% | %1% | %1% | `src/ui/FormatHelper.cpp` |
| Next: No upcoming interviews | Далі: немає запланованих співбесід | Далее: нет запланированных собеседований | `src/ui/FormatHelper.cpp` |
| Next: %1 %2 %3 | Далі: %1 %2 %3 | Далее: %1 %2 %3 | `src/ui/FormatHelper.cpp` |
| JobPrep keeps running in the tray; use the tray menu to quit | JobPrep продовжує працювати в треї; скористайтеся меню трею для виходу | JobPrep продолжает работать в трее; используйте меню трея для выхода | `src/ui/MainWindow.cpp` |
| Next: No upcoming interviews | Далі: немає запланованих співбесід | Далее: нет запланированных собеседований | `src/ui/MainWindow.cpp` |
| Log a study session | Записати сесію навчання | Записать занятие | `src/ui/MainWindow.cpp` |
| Use the search box on this page, Ctrl+F | Скористайтеся полем пошуку на цій сторінці, Ctrl+F | Используйте поле поиска на этой странице, Ctrl+F | `src/ui/MainWindow.cpp` |
| Cannot open the database | Не вдалося відкрити базу даних | Не удалось открыть базу данных | `src/main.cpp` |
| JobPrep could not open its data file.<br><br>%1 | JobPrep не зміг відкрити свій файл даних.<br><br>%1 | JobPrep не смог открыть свой файл данных.<br><br>%1 | `src/main.cpp` |
| Would you like to start with a sample study plan?<br><br>It adds the C++ and English tracks with their topics and checklists. | Бажаєте розпочати зі зразкового плану навчання?<br><br>Він додасть напрями C++ та English з відповідними темами та чеклістами. | Хотите начать с примерного учебного плана?<br><br>Он добавит направления C++ и English с соответствующими темами и чеклистами. | `src/main.cpp` |
| The sample data could not be loaded.<br><br>%1 | Не вдалося завантажити демонстраційні дані.<br><br>%1 | Не удалось загрузить демонстрационные данные.<br><br>%1 | `src/main.cpp` |
| show or hide this layer | показати або приховати цей шар | показать или скрыть этот слой | `src/main.cpp` |
| Press Alt+Down or F4 to open calendar | Натисніть Alt+Down або F4, щоб відкрити календар | Нажмите Alt+Down или F4, чтобы открыть календарь | `src/ui/widgets/SingleTabDateTime.h` |

