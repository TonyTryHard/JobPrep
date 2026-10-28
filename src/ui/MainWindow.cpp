#include "ui/MainWindow.h"
#include <QCloseEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/TopicRepository.h"
#include "services/INotifier.h"
#include "services/SettingsService.h"
#include "ui/DashboardController.h"
#include "ui/DataToolsController.h"
#include "ui/FormatHelper.h"
#include "ui/PageBase.h"
#include "ui/Sidebar.h"
#include "ui/dialogs/SessionDialog.h"
#include "ui/pages/CalendarPage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/JobsPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/StudyPage.h"
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui {

MainWindow::MainWindow(App::AppContext& ctx, bool trayAvailable, QWidget* parent)
    : QMainWindow(parent), m_ctx(ctx), m_trayAvailable(trayAvailable) {
    setObjectName(u"MainWindow"_s);
    setWindowTitle(u"JobPrep"_s);
    setMinimumSize(1100, 700);

    m_dashboardController = std::make_unique<DashboardController>(
        m_ctx.statsService(), m_ctx.agendaService(), m_ctx.settings(), m_ctx.dayWatcher(),
        m_ctx.applications(), m_ctx.interviews(), m_ctx.sessions(), m_ctx.topics(),
        m_ctx.tracks(), this);

    setupUi();
    setupShortcuts();
    setupConnections();

    // Restore persisted geometry and sidebar state
    const QByteArray geo = m_ctx.settings().windowGeometry();
    if (!geo.isEmpty()) {
        restoreGeometry(geo);
    } else {
        resize(1100, 700);
    }

    const QByteArray state = m_ctx.settings().windowState();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    m_sidebar->setCollapsed(m_ctx.settings().isSidebarCollapsed());

    updateHeader(0);
    updateIcons();
    updateQuitOnLastWindowClosed();
}

MainWindow::~MainWindow() = default;

void MainWindow::setTrayAvailable(bool available) {
    if (m_trayAvailable == available) return;
    m_trayAvailable = available;
    if (m_settingsPage) {
        m_settingsPage->setTrayAvailable(available);
    }
    updateQuitOnLastWindowClosed();
}

void MainWindow::setTrayMessageCallback(std::function<void(const QString& title, const QString& message)> callback) {
    m_trayMessageCallback = std::move(callback);
}

void MainWindow::setLiveTrayCheck(std::function<bool()> check) {
    m_liveTrayCheck = std::move(check);
}

bool MainWindow::isTrayAvailableLive() const {
    if (m_liveTrayCheck) {
        return m_liveTrayCheck();
    }
    return m_trayAvailable;
}

void MainWindow::updateQuitOnLastWindowClosed() {
    const bool keepRunningInTray = isTrayAvailableLive() && m_ctx.settings().closeToTray();
    qApp->setQuitOnLastWindowClosed(!keepRunningInTray);
}

void MainWindow::flushStateAndEdits() {
    for (auto* page : std::as_const(m_pages)) {
        page->flushEdits();
    }
    m_ctx.settings().setWindowGeometry(saveGeometry());
    m_ctx.settings().setWindowState(saveState());
    if (m_dashboardController) {
        m_dashboardController->stop();
    }
}

void MainWindow::prepareForShutdown() {
    m_ctx.restoreService().waitForRestore(5000);
    flushStateAndEdits();
}

void MainWindow::setReminderService(Services::ReminderService* service) {
    if (m_dataToolsController) {
        m_dataToolsController->setReminderService(service);
    }
}

void MainWindow::quitApplication() {
    m_isQuitting = true;
    prepareForShutdown();
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (qGuiApp && qGuiApp->isSavingSession()) {
        prepareForShutdown();
        QMainWindow::closeEvent(event);
        return;
    }

    if (!m_isQuitting && isTrayAvailableLive() && m_ctx.settings().closeToTray()) {
        event->ignore();
        flushStateAndEdits();
        hide();
        if (!m_ctx.settings().trayHintShown()) {
            if (m_trayMessageCallback) {
                m_trayMessageCallback(
                    tr("JobPrep"),
                    tr("JobPrep keeps running in the tray; use the tray menu to quit")
                );
                m_ctx.settings().setTrayHintShown(true);
            }
        }
        return;
    }

    prepareForShutdown();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Sidebar
    m_sidebar = new Sidebar(m_ctx.themeManager(), centralWidget);
    rootLayout->addWidget(m_sidebar);

    // Right-hand area (Header + Content stack)
    auto* rightContainer = new QWidget(centralWidget);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Header bar
    auto* header = new QWidget(rightContainer);
    header->setObjectName(u"HeaderWidget"_s);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 0, 24, 0);
    headerLayout->setSpacing(16);

    m_pageTitleLabel = new QLabel(header);
    m_pageTitleLabel->setObjectName(u"HeaderTitle"_s);
    headerLayout->addWidget(m_pageTitleLabel, 1, Qt::AlignVCenter | Qt::AlignLeft);

    m_searchEdit = new QLineEdit(header);
    m_searchEdit->setObjectName(u"HeaderSearchEdit"_s);
    m_searchEdit->setPlaceholderText(tr("Search... (Ctrl+F)"));
    m_searchEdit->setClearButtonEnabled(true);
    headerLayout->addWidget(m_searchEdit, 0, Qt::AlignVCenter);

    // A menu, not a bare button: each entry point can arrive in a later milestone and
    // must not be a dead or ambiguous button until then.
    m_newBtn = new QToolButton(header);
    m_newBtn->setObjectName(u"HeaderNewBtn"_s);
    m_newBtn->setText(tr("+ New"));
    m_newBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_newBtn->setCursor(Qt::PointingHandCursor);
    m_newBtn->setPopupMode(QToolButton::InstantPopup);
    m_newBtn->setMenu(setupNewMenu(header));
    m_newBtn->setToolTip(tr("Add something new"));
    headerLayout->addWidget(m_newBtn, 0, Qt::AlignVCenter);

    rightLayout->addWidget(header);

    // Pages stack
    m_pageStack = new QStackedWidget(rightContainer);
    m_pageStack->setObjectName(u"PageStack"_s);

    m_homePage = new Pages::HomePage(*m_dashboardController, m_ctx.themeManager(), m_pageStack);
    m_pages.append(m_homePage);
    m_homeIndex = static_cast<int>(m_pages.size()) - 1;
    m_studyPage = new Pages::StudyPage(m_ctx.topics(), m_ctx.tracks(), m_ctx.sessions(),
                                       m_ctx.settings(), m_ctx.themeManager(), m_pageStack);
    m_pages.append(m_studyPage);
    m_studyIndex = static_cast<int>(m_pages.size()) - 1;
    m_jobsPage = new Pages::JobsPage(m_ctx.applications(), m_ctx.interviews(),
                                     m_ctx.exportService(), m_pageStack);
    m_pages.append(m_jobsPage);
    m_jobsIndex = static_cast<int>(m_pages.size()) - 1;
    m_calendarPage = new Pages::CalendarPage(
        m_ctx.interviews(), m_ctx.applications(), m_ctx.topics(), m_ctx.tracks(),
        m_ctx.agendaService(), m_ctx.settings(), m_ctx.themeManager(), m_pageStack);
    m_pages.append(m_calendarPage);
    m_calendarIndex = static_cast<int>(m_pages.size()) - 1;
    m_dataToolsController = std::make_unique<DataToolsController>(
        m_ctx.database(), m_ctx.exportService(), m_ctx.restoreService(), m_ctx.seedService(),
        m_ctx.tracks(), m_ctx.topics(), m_ctx.sessions(), m_ctx.applications(), m_ctx.interviews(),
        nullptr, this);
    m_dataToolsController->setFlushCallback([this] { flushStateAndEdits(); });
    m_dataToolsController->setIsEmptyCallback([this] { return m_ctx.isDataEmpty(); });

    m_settingsPage = new Pages::SettingsPage(m_ctx.settings(), m_ctx.themeManager(), m_trayAvailable,
                                             m_pageStack, m_dataToolsController.get());
    m_pages.append(m_settingsPage);
    m_settingsIndex = static_cast<int>(m_pages.size()) - 1;

    for (auto* page : m_pages) {
        m_pageStack->addWidget(page);
    }

    rightLayout->addWidget(m_pageStack, 1);
    rootLayout->addWidget(rightContainer, 1);

    // Status bar setup
    auto* status = statusBar();
    status->setSizeGripEnabled(false);

    m_statusStreakLabel = new QLabel(tr("0-day streak"), this);
    m_statusProcessesLabel = new QLabel(tr("0 active processes"), this);
    m_statusNextInterviewLabel = new QLabel(tr("Next: No upcoming interviews"), this);

    status->addWidget(m_statusStreakLabel);
    status->addWidget(new QLabel(u" | "_s, this));
    status->addWidget(m_statusProcessesLabel);
    status->addWidget(new QLabel(u" | "_s, this));
    status->addWidget(m_statusNextInterviewLabel);

    connect(m_dashboardController.get(), &DashboardController::snapshotChanged, this,
            [this](const DashboardSnapshot& snap) {
                m_statusStreakLabel->setText(FormatHelper::formatStreak(snap.stats.studyStreakDays));
                m_statusProcessesLabel->setText(FormatHelper::formatActiveProcesses(snap.stats.activeProcesses));
                m_statusNextInterviewLabel->setText(
                    FormatHelper::formatNextInterview(snap.nextInterview));
            });

    const auto& initialSnap = m_dashboardController->snapshot();
    m_statusStreakLabel->setText(FormatHelper::formatStreak(initialSnap.stats.studyStreakDays));
    m_statusProcessesLabel->setText(FormatHelper::formatActiveProcesses(initialSnap.stats.activeProcesses));
    m_statusNextInterviewLabel->setText(
        FormatHelper::formatNextInterview(initialSnap.nextInterview));
}

void MainWindow::setupShortcuts() {
    // Ctrl+1 through Ctrl+4 for main pages, Ctrl+, for Settings
    for (int i = 0; i < 4; ++i) {
        auto* shortcut = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), this);
        connect(shortcut, &QShortcut::activated, this, [this, i]() {
            m_sidebar->setCurrentPage(i);
            updateHeader(i);
        });
    }

    auto* prefsShortcut = new QShortcut(QKeySequence::Preferences, this);
    connect(prefsShortcut, &QShortcut::activated, this, [this]() {
        m_sidebar->setCurrentPage(4);
        updateHeader(4);
    });

    // On Linux/X11, QKeySequence::Preferences maps to Qt::Key_Settings rather than
    // Ctrl+,, but SPEC §3.7 explicitly specifies Ctrl+, for settings.
    auto* ctrlCommaShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this);
    connect(ctrlCommaShortcut, &QShortcut::activated, this, [this]() {
        m_sidebar->setCurrentPage(4);
        updateHeader(4);
    });

    // Shortcuts live here only, never on a widget, so none of them is ambiguous.
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, &MainWindow::focusCurrentPageSearch);

    auto* newShortcut = new QShortcut(QKeySequence::New, this);
    connect(newShortcut, &QShortcut::activated, this, &MainWindow::triggerCurrentPageNew);
}

QMenu* MainWindow::setupNewMenu(QWidget* parent) {
    auto* menu = new QMenu(parent);
    menu->setObjectName(u"HeaderNewMenu"_s);

    QAction* application = menu->addAction(tr("Application"));
    application->setObjectName(u"HeaderNewApplication"_s);
    connect(application, &QAction::triggered, this, &MainWindow::newApplicationFromHeader);

    QAction* interview = menu->addAction(tr("Interview"));
    interview->setObjectName(u"HeaderNewInterview"_s);
    connect(interview, &QAction::triggered, this, &MainWindow::newInterviewFromHeader);

    QAction* topic = menu->addAction(tr("Topic"));
    topic->setObjectName(u"HeaderNewTopic"_s);
    connect(topic, &QAction::triggered, this, &MainWindow::newTopicFromHeader);

    QAction* session = menu->addAction(tr("Study session"));
    session->setObjectName(u"HeaderNewSession"_s);
    session->setEnabled(true);
    session->setToolTip(tr("Log a study session"));
    connect(session, &QAction::triggered, this, &MainWindow::newSessionFromHeader);
    return menu;
}

void MainWindow::goToPage(int pageIndex) {
    m_sidebar->setCurrentPage(pageIndex);
    updateHeader(pageIndex);
}

void MainWindow::focusCurrentPageSearch() {
    auto* page = m_pages.value(m_pageStack->currentIndex(), nullptr);
    if (!page) return;
    page->focusSearch();
}

void MainWindow::triggerCurrentPageNew() {
    auto* page = m_pages.value(m_pageStack->currentIndex(), nullptr);
    if (!page) return;
    page->triggerNew();
}

void MainWindow::newApplicationFromHeader() {
    if (m_jobsIndex < 0) return;
    goToPage(m_jobsIndex);
    m_jobsPage->newApplication();
}

void MainWindow::newInterviewFromHeader() {
    if (m_calendarIndex < 0) return;
    goToPage(m_calendarIndex);
    m_calendarPage->newInterview(m_calendarPage->selectedDay());
}

void MainWindow::newTopicFromHeader() {
    if (m_studyIndex < 0) return;
    goToPage(m_studyIndex);
    m_studyPage->newTopic();
}

void MainWindow::newSessionFromHeader() {
    Dialogs::SessionDialog dialog(m_ctx.sessions(), m_ctx.topics(), this);
    if (m_pageStack->currentIndex() == m_studyIndex && m_studyPage) {
        if (const auto topicId = m_studyPage->selectedTopicId()) {
            dialog.setTopicId(*topicId);
        }
    }
    dialog.exec();
}

void MainWindow::openTopic(int topicId) {
    const auto topic = m_ctx.topics().byId(topicId);
    if (!topic) {
        statusBar()->showMessage(tr("Topic not found"), 3000);
        return;
    }
    if (m_studyIndex < 0) return;
    goToPage(m_studyIndex);
    m_studyPage->selectTopic(topicId);
}

void MainWindow::openInterview(int interviewId) {
    const auto iv = m_ctx.interviews().byId(interviewId);
    if (!iv) {
        statusBar()->showMessage(tr("Interview not found"), 3000);
        return;
    }
    if (m_calendarIndex < 0) return;
    goToPage(m_calendarIndex);
    m_calendarPage->openInterview(interviewId);
}

void MainWindow::openApplication(int applicationId) {
    const auto app = m_ctx.applications().byId(applicationId);
    if (!app) {
        statusBar()->showMessage(tr("Application not found"), 3000);
        return;
    }
    if (m_jobsIndex < 0) return;
    goToPage(m_jobsIndex);
    m_jobsPage->openApplication(applicationId);
}

Theme::ThemeManager& MainWindow::themeManager() const {
    return m_ctx.themeManager();
}

void MainWindow::bringToFront() {
    if (isHidden() || !isVisible()) {
        show();
    }
    if (isMinimized()) {
        showNormal();
    }
    raise();
    activateWindow();
}

void MainWindow::routeNotification(const Services::Notification& notification) {
    switch (notification.kind) {
        case Services::NotificationKind::Interview:
            openInterview(notification.refId);
            break;
        case Services::NotificationKind::FollowUp:
            openApplication(notification.refId);
            break;
        case Services::NotificationKind::OverdueFollowUps:
            if (notification.refId > 0) {
                openApplication(notification.refId);
            } else if (m_jobsIndex >= 0) {
                goToPage(m_jobsIndex);
            }
            break;
    }
}

void MainWindow::openItemFromNotification(const Services::Notification& notification) {
    bringToFront();
    routeNotification(notification);
}

void MainWindow::setupConnections() {
    connect(m_sidebar, &Sidebar::pageSelected, this, [this](int index) {
        updateHeader(index);
    });

    if (m_homePage) {
        connect(m_homePage, &Pages::HomePage::addApplicationRequested, this,
                &MainWindow::newApplicationFromHeader);
        connect(m_homePage, &Pages::HomePage::addTopicRequested, this,
                &MainWindow::newTopicFromHeader);
        connect(m_homePage, &Pages::HomePage::interviewActivated, this,
                &MainWindow::openInterview);
        connect(m_homePage, &Pages::HomePage::applicationActivated, this,
                &MainWindow::openApplication);
        connect(m_homePage, &Pages::HomePage::topicActivated, this,
                &MainWindow::openTopic);
    }

    connect(m_calendarPage, &Pages::CalendarPage::studyTopicActivated, this,
            &MainWindow::openTopic);

    // The header box owns no filter state: it forwards to the page, which mirrors the
    // text back under a blocker.
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_pageStack->currentIndex() == m_studyIndex) return;
        if (m_jobsIndex < 0) return;
        if (m_pageStack->currentIndex() != m_jobsIndex) goToPage(m_jobsIndex);
        m_jobsPage->setSearchText(text);
    });

    connect(m_sidebar, &Sidebar::collapseToggled, this, [this](bool collapsed) {
        m_ctx.settings().setSidebarCollapsed(collapsed);
    });

    connect(&m_ctx.themeManager(), &Theme::ThemeManager::themeChanged, this, [this]() {
        updateIcons();
    });

    connect(&m_ctx.settings(), &Services::SettingsService::closeToTrayChanged, this, [this](bool) {
        updateQuitOnLastWindowClosed();
    });
}

void MainWindow::updateHeader(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_pages.size()) return;

    m_pageStack->setCurrentIndex(pageIndex);
    auto* activePage = m_pages[pageIndex];
    m_pageTitleLabel->setText(activePage->pageTitle());
    activePage->onActivated();

    if (pageIndex == m_studyIndex) {
        m_searchEdit->setEnabled(false);
        m_searchEdit->setToolTip(tr("Use the search box on this page, Ctrl+F"));
    } else {
        m_searchEdit->setEnabled(true);
        m_searchEdit->setToolTip(QString());
    }
}

void MainWindow::updateIcons() {
    const auto tokens = m_ctx.themeManager().currentTokens();
    const QIcon searchIcon = Theme::IconProvider::icon(u"search"_s, tokens.textMuted);

    if (m_searchAction) {
        m_searchAction->setIcon(searchIcon);
    } else {
        m_searchAction = m_searchEdit->addAction(searchIcon, QLineEdit::LeadingPosition);
    }

    const QIcon plusIcon = Theme::IconProvider::icon(u"plus"_s, tokens.accentText);
    m_newBtn->setIcon(plusIcon);
}

}  // namespace JobPrep::Ui
