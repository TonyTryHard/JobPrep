# Task: a GitHub README worth showing to people

Read `AGENTS.md`, `docs/SPEC.md`, `docs/RUNNING.md`, `docs/reports/M7.md`, `.github/workflows/ci.yml`, `CMakeLists.txt`, `CMakePresets.json`. Local commits only, never push, never tag. Never run the app binary without an isolated `HOME`/`XDG_*` (and on Linux never touch `~/.local/share/JobPrep`). Raw output with the exact command above it, nothing trimmed, `(no output)` for empty, hashes only from `git log --oneline`, totals by command, scratch files and throw-away worktrees outside the repository (`git worktree list` raw at the end). Anything about Windows you did not run is "not verified by agent". No code changes in this task except what section 5 allows.

## Goal

A top-level `README.md` in English, written for a stranger who opens the repository: a developer who may want to build it, and a person who only wants to see what it is. It must be accurate, concise, and good-looking on GitHub. It is not a changelog and not marketing copy: no inflated claims, no filler, no "blazing fast", no emoji decoration (at most none or very few plain section markers).

## 1. Facts first (do before writing)

Every statement in the README must come from the code, `SPEC.md`, `RUNNING.md`, the CI workflow or a command you ran. Collect and show raw, then write:

1. What the app is and does: pages (Home, Study, Jobs, Calendar, Settings) and their real features (study topics and checklists, kanban/table of topics, sessions and weekly goal, applications and statuses, interviews and follow-ups, calendar and agenda, reminders and in-app notifications, backup/restore JSON, CSV and ICS export, sample data, light/dark theme, keyboard shortcuts, languages English/Ukrainian/Russian with restart on change). Use `grep`/`sed` on `docs/SPEC.md`, and the page sources, and say in the report where each README feature claim comes from (a table: claim, source file or command).
2. Tech facts: C++20, Qt 6 Widgets, minimum Qt version (`grep -n "find_package(Qt6" CMakeLists.txt`), SQLite storage, CMake, Ninja, presets (`cmake --list-presets`), supported platforms, compilers in CI (`ci.yml`), test count (`ctest -N` raw) and test suites by area.
3. Build and run steps that actually work: Linux (apt packages from `docs/RUNNING.md` and the CI install step, preset, run command) and Windows (Qt kit, MSVC, preset or the generic Release configure line, and the deployment with `windeployqt`; the real Windows run was verified by the user, the instructions you write are checked against `RUNNING.md` and CI only: say so in the report). Execute the Linux steps in a throw-away clone outside the repository and show the raw result (configure, build, tests; time them), or say exactly what you could not run.
4. Where data lives: read it from the code (`QStandardPaths::AppDataLocation`, organisation and application names in `main.cpp`) and state the Linux and Windows paths; the Windows path is derived from the code, not run: mark it as such in the report. Mention backup/restore in the Data section.
5. The repository coordinates for badges and links: `git remote -v` and the CI workflow file name and workflow `name:`. Do not guess the owner, name or default branch.
6. Licence: `ls LICENSE* COPYING*` raw. If there is no licence file, do **not** invent or add one; put a line in the report for the user, and in the README leave the licence section out (or write "No licence file yet" only if the user's choice is unknown; prefer leaving it out and flagging).

## 2. Screenshots

- Real screenshots of the user's Windows PC are better, but he is not available now. Produce clean ones with the existing offscreen test harness or a scratch program outside the repository, using **sample data (English), an isolated `HOME`/`XDG_*` and no real data**: Home, Study (a topic with checklist open), Jobs, Calendar, Settings in Light, plus one Dark. 100% scale, a sensible window size (about 1280x800), a readable font (state the font family used).
- Save as PNG into `docs/images/` (optimised, each under about 250 KB, total shown by `du -sh docs/images`; check `ls -la docs/images` raw) and reference them with relative paths. Say plainly in the report that they are offscreen renders, not the user's screen, so he can replace them with real ones; name the files so replacing is one copy (`screenshot-home.png`, ...).
- Check every image for private data: no real names, e-mails, company names from real life, paths from your machine or the user's name. Show how you checked (the sample data source and a visual check of each file with the Read tool).
- Do not touch `docs/reports/images/`.

## 3. README structure (adapt, do not pad)

1. Title, one-sentence description, CI badge (real workflow, real repo), optional Qt/C++ badges only if they are accurate and cheap (no external badge service that needs an account).
2. A short "What it is" paragraph and a feature list of the real features (grouped, about 8 to 12 bullets), followed by a screenshots block (a small table or two rows, not a gallery of ten).
3. Languages and themes (English, Ukrainian, Russian; system language by default; restart on change; light/dark).
4. Quick start: Linux and Windows, with the exact commands; requirements first (Qt 6.8 or newer with the right version string from CMake, compiler, CMake, Ninja).
5. Running it and where the data is (paths, backup/restore, export; the Windows deployment recipe with `windeployqt` in about five lines; a note that dev builds and a deployed copy share one database).
6. Keyboard shortcuts: a compact table from `SPEC.md`/code (do not invent any; show the grep that proves each).
7. Project layout in one short tree (top-level directories only, one line each) and the architecture in three or four sentences (core library, UI, data layer, tests).
8. Testing: how to run the quality gate (`./scripts/check.sh`, `scripts/check.ps1`), what CI runs (jobs by real name), the translation sync check, the pseudo-translation and layout tests in one sentence each.
9. Status and roadmap: a short honest status (milestones M0 to M7 done; the real known limits from `docs/reports/M7.md` and SPEC; what is not done, for example no cloud sync) taken from the documents, not invented.
10. Contributing/Notes: only what is true (for example "this is a personal project; issues welcome" is a decision for the user: do **not** write it; flag it in the report as a question).

Style: plain GitHub Markdown, headings no deeper than `###`, tables only where they help, code fences with language tags, lines not needlessly long, no trailing marketing sentences. Verify that every relative link and image path resolves (`grep -o` the links and test each with `ls`, raw), and that the Markdown renders: use a Markdown linter if one is installed, or render it with a tool available locally (say which) and read the HTML for broken structure; show that check raw.

## 4. Repository hygiene report (report only, do not change)

The repository is going to be shown to people. List, without moving or deleting anything:

- `git ls-files docs | head -100` and `git ls-files docs | wc -l` raw, and which of `docs/prompts/`, `docs/reports/`, `docs/plans/` hold working notes that the user may want to keep or move out of a public view;
- private-looking content in tracked files: `git grep -n "/home/\|C:\\\\Users\|@gmail\|@users.noreply" -- . ':!docs/reports'` raw, then the same with `docs/reports` included and counted (`| wc -l`), with a verdict per hit class (needs removal / harmless);
- large or binary files in the history that should not be public: `git ls-files -z | xargs -0 du -ch | tail -1` and the ten largest tracked files raw;
- whether `AGENTS.md` and the prompt files name an AI agent product (the user decides if that stays; just list the files).

## 5. Allowed edits besides the README

- `docs/RUNNING.md`: fix any statement you found to be wrong or out of date while checking the README (each fix listed with the command that proved it); link from the README to `docs/RUNNING.md` and `docs/SPEC.md` for details instead of duplicating them.
- `.gitignore`: nothing.
- No source code, tests, CI or build file changes. If you find a defect or a missing piece (for example no Release preset, or no licence), report it; do not fix it here.

## Commits (separate)

(1) screenshots, (2) `README.md`, (3) `docs/RUNNING.md` fixes if any, (4) report last: `docs/reports/README-task.md` (facts as given; the claim-to-source table; raw evidence of the Linux build from a clean clone and the link/Markdown checks; the hygiene lists of section 4; **Not verified by agent, user to check:** the Windows instructions on his PC, the screenshots against his real screen, the badge on GitHub after he pushes, the tone of the text; questions for the user: licence, "contributing" line, whether to hide `docs/prompts`/`docs/reports`, whether to name the agent), the final `git status --short`, `git worktree list`, `git log --oneline` of this work including the report commit, and a full `CMAKE_BUILD_PARALLEL_LEVEL=2 ./scripts/check.sh` raw run to show that nothing broke.
