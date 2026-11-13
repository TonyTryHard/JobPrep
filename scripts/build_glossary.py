#!/usr/bin/env python3
import csv
import os

proposals = {
    'Application': ('Заявка', 'Заявка', 'User decision: unified term for job application'),
    'Applications': ('Заявки', 'Заявки', 'Plural of Application'),
    'Job application': ('Заявка на роботу', 'Заявка на работу', 'Full form'),
    'Job Applications': ('Заявки на роботу', 'Заявки на работу', 'Section title'),
    'Follow-up': ('Фолоу-ап', 'Фоллоу-ап', 'Distinct from Reminder (Нагадування / Напоминание)'),
    'Follow-ups': ('Фолоу-апи', 'Фоллоу-апы', 'Plural of Follow-up'),
    'Reminder': ('Нагадування', 'Напоминание', 'System/service reminder notification'),
    'Reminders': ('Нагадування', 'Напоминания', 'Plural'),
    'Target': ('Ціль', 'Цель', 'Date-bound goal'),
    'Study target': ('Навчальна ціль', 'Учебная цель', 'Agenda study milestone'),
    'Study targets': ('Навчальні цілі', 'Учебные цели', 'Layer chip in Calendar'),
    'Status': ('Статус', 'Статус', 'Disambiguated between hiring stage and study topic'),
    'Applied': ('Подано', 'Подано', 'Hiring stage: application submitted'),
    'Today': ('Сьогодні', 'Сегодня', 'Current day'),
    'Open': ('Відкрити', 'Открыть', 'Action button to open external link'),
    'JobPrep': ('JobPrep', 'JobPrep', 'Untranslated program name'),
    'HR': ('HR', 'HR', 'Technical/domain acronym untranslated'),
    'HR Screen': ('HR-скринінг', 'HR-скрининг', 'Interview stage'),
    'HR screen': ('HR-скринінг', 'HR-скрининг', 'Interview type'),
    'Technical': ('Технічне', 'Техническое', 'Interview type or stage'),
    'Final': ('Фінальне', 'Финальное', 'Interview stage'),
    'Offer': ('Офер', 'Оффер', 'Hiring stage'),
    'Accepted': ('Прийнято', 'Принято', 'Hiring stage outcome'),
    'Rejected': ('Відхилено', 'Отклонено', 'Hiring stage outcome'),
    'Withdrawn': ('Відкликано', 'Отозвано', 'Application withdrawn'),
    'Ghosted': ('Без відповіді', 'Без ответа', 'Application ghosted'),
    'Wishlist': ('Список бажань', 'Список желаний', 'Application wishlist'),
    'Backlog': ('Беклог', 'Бэклог', 'Topic board status'),
    'In Progress': ('В процесі', 'В процессе', 'Topic board status'),
    'Review': ('На перегляді', 'На проверке', 'Topic board status'),
    'Done': ('Виконано', 'Выполнено', 'Topic board status'),
    'Low': ('Низький', 'Низкий', 'Priority'),
    'Normal': ('Звичайний', 'Обычный', 'Priority'),
    'High': ('Високий', 'Высокий', 'Priority'),
    'Live coding': ('Лайвкодинг', 'Лайвкодинг', 'Interview type'),
    'System design': ('Системний дизайн', 'Системный дизайн', 'Interview type'),
    'Behavioral': ('Поведінкове', 'Поведенческое', 'Interview type'),
    'Other': ('Інше', 'Другое', 'Category/type'),
    'Pending': ('Очікується', 'Ожидается', 'Interview outcome'),
    'Passed': ('Пройдено', 'Пройдено', 'Interview outcome'),
    'Failed': ('Не пройдено', 'Не пройдено', 'Interview outcome'),
    'Cancelled': ('Скасовано', 'Отменено', 'Interview outcome'),
    'Save': ('Зберегти', 'Сохранить', 'Primary action button'),
    'Cancel': ('Скасувати', 'Отмена', 'Secondary action button'),
    'Close': ('Закрити', 'Закрыть', 'Close dialog or panel'),
    'Delete': ('Видалити', 'Удалить', 'Destructive action'),
    'Edit': ('Редагувати', 'Редактировать', 'Action'),
    'New': ('Створити', 'Создать', 'Create action'),
    'Search': ('Пошук', 'Поиск', 'Filter field placeholder'),
    'Filter': ('Фільтр', 'Фильтр', 'Filter control'),
    'Track': ('Напрям', 'Направление', 'Study track'),
    'Tracks': ('Напрями', 'Направления', 'Plural of study track'),
    'Topic': ('Тема', 'Тема', 'Study topic'),
    'Topics': ('Теми', 'Темы', 'Plural of study topic'),
    'Interview': ('Співбесіда', 'Собеседование', 'Job interview'),
    'Interviews': ('Співбесіди', 'Собеседования', 'Plural of interview'),
    'Calendar': ('Календар', 'Календарь', 'Navigation page'),
    'Settings': ('Налаштування', 'Настройки', 'Navigation page'),
    'Home': ('Головна', 'Главная', 'Dashboard page'),
    'Study': ('Навчання', 'Обучение', 'Study page'),
    'Jobs': ('Робота', 'Работа', 'Jobs page'),
    'Company': ('Компанія', 'Компания', 'Application field'),
    'Position': ('Посада', 'Должность', 'Application field'),
    'Salary': ('Зарплата', 'Зарплата', 'Application field'),
    'Location': ('Локація', 'Локация', 'Application field'),
    'CSV': ('CSV', 'CSV', 'Technical token untranslated'),
    'ICS': ('ICS', 'ICS', 'Technical token untranslated'),
    'JSON': ('JSON', 'JSON', 'Technical token untranslated'),
}

with open('docs/i18n-glossary-raw.tsv', 'r', encoding='utf-8') as f:
    reader = csv.DictReader(f, delimiter='\t')
    rows = list(reader)

lines = []
lines.append('# JobPrep Translation Glossary & Terminology Proposals')
lines.append('')
lines.append('> [!IMPORTANT]')
lines.append('> Every row in this document is a **draft proposal for the user\'s review and approval**.')
lines.append('> No translations have been written into `jobprep_uk.ts` or `jobprep_ru.ts` in Part C (deferred to Part D).')
lines.append('')
lines.append('## 1. Terminology Decisions & Conflict Resolutions')
lines.append('')
lines.append('- **`Application` (Job application):** Unified as **`Заявка`** in both Ukrainian and Russian per user specification.')
lines.append('- **`Follow-up` vs `Reminder`:** `Follow-up` is **`Фолоу-ап`** (uk) / **`Фоллоу-ап`** (ru). In contrast, `Reminder` refers to the system reminder service notification (**`Нагадування`** / **`Напоминание`**).')
lines.append('- **`Target`:** Represents a date-bound study topic goal (**`Ціль`** / **`Цель`**).')
lines.append('- **Disambiguated Terms:** Disambiguated in source code with `//:` translator comments and distinct contexts:')
lines.append('  - `Application`: menu action `tr("Application")` with comment `//: Menu action to create a new job application (not program)`')
lines.append('  - `Status`: column header `tr("Status")` (`//: Header for job application stage column`) vs `TopicListModel` (`//: Header for study topic status column`) vs `ApplicationDialog` (`//: Form field label for job application hiring stage`)')
lines.append('  - `Applied`: table header `tr("Applied")` (`//: Header for date when job application was submitted`) vs enum label `StatusStyle::label` (`//: Application hiring stage: application has been submitted`)')
lines.append('  - `Today`: navigation button in `CalendarPage` (`//: Button action to navigate calendar view to current day`) vs dashboard card in `HomePage` (`//: Dashboard card title showing today\'s agenda items`)')
lines.append('  - `Open`: action button in `TopicDetailPanel` (`//: Button action to open link in external web browser`)')
lines.append('- **Untranslated Tokens:** Program name `JobPrep`, technical acronyms `HR`, file formats `CSV`, `ICS`, `JSON` remain strictly untranslated.')
lines.append('- **Mnemonics:** Ukrainian and Russian translations intentionally omit all `&` keyboard mnemonics (no `&` in Cyrillic UI).')
lines.append('')
lines.append('## 2. Extraction Methodology')
lines.append('')
lines.append('Derived from source code via automated regex scan over all `tr()` and `translate()` calls:')
lines.append('```bash')
lines.append('python3 scripts/build_glossary.py')
lines.append('wc -l docs/i18n-glossary-raw.tsv  # 322 lines (321 distinct short terms + header)')
lines.append('```')
lines.append('')
lines.append('## 3. Glossary Table (Draft Proposals for User Review)')
lines.append('')
lines.append('| English UI String | Context | Ukrainian Proposal | Russian Proposal | Notes |')
lines.append('| :--- | :--- | :--- | :--- | :--- |')

seen_terms = set()
for r in rows:
    src = r['Source']
    ctx = r['Context']
    comm = r['Comment']
    
    clean_src = src.replace('&', '')
    if clean_src in proposals:
        uk, ru, note = proposals[clean_src]
    elif src in proposals:
        uk, ru, note = proposals[src]
    else:
        uk = f'*{clean_src}*'
        ru = f'*{clean_src}*'
        note = 'Proposal pending review'
    
    if comm:
        note += f' (Translator comment: {comm})'
    
    key = (src, ctx)
    if key not in seen_terms:
        seen_terms.add(key)
        lines.append(f'| `{src}` | `{ctx}` | {uk} | {ru} | {note} |')

with open('docs/i18n-glossary.md', 'w', encoding='utf-8') as out:
    out.write('\n'.join(lines) + '\n')

print(f'Wrote {len(seen_terms)} terms to docs/i18n-glossary.md')
