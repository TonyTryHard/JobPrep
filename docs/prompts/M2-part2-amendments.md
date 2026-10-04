Plan approved with these amendments:
1. Real-launch check: set BOTH XDG_DATA_HOME and XDG_CONFIG_HOME to fresh temp dirs (never touch the user's real config).
2. Pending notifications: QList<QPointer<Repository>> with dedupe (ordered, not a QSet); forward-declare Repository in Database.h; flush by moving the list to a local before emitting; reset m_outerFailed in begin() too.
3. Modal test helper supports accept AND reject; add negative-path tests (Delete -> No keeps the row; Cancel with edits -> keep editing).
4. Report must say "not verified by agent, user to check" for: status badge contrast in light/dark, and CSV opening in a spreadsheet app. Do not claim visual verification.
5. Three local commits, each after ./scripts/check.sh passes: Part A; B1 (JobsPage + ApplicationDialog + their smoke tests); B2 (MainWindow header integration + smoke tests). If quota runs out, stop after the last green commit and say exactly what remains.
6. Update tests that read the status DisplayRole to use StatusRole. CSV headers: fixed English names (no tr()), no id, Updated once; list them in the report.
7. Enter/double-click via activated/doubleClicked; Delete handled only on the table view (test: Delete in the search box does nothing to the table). Footer counts all applications. Dialog e-mail check warns only. Repositories return before opening a transaction when there is nothing to do (e.g. unknown id).
