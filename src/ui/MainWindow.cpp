#include "ui/MainWindow.h"
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include "app/AppContext.h"
#include "services/SettingsService.h"
#include "ui/PageBase.h"
#include "ui/Sidebar.h"
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

MainWindow::MainWindow(App::AppContext& ctx, QWidget* parent)
    : QMainWindow(parent), m_ctx(ctx) {
    setObjectName(u"MainWindow"_s);
    setWindowTitle(u"JobPrep"_s);
    setMinimumSize(1100, 700);

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
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    m_ctx.settings().setWindowGeometry(saveGeometry());
    m_ctx.settings().setWindowState(saveState());
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

    m_newBtn = new QPushButton(header);
    m_newBtn->setObjectName(u"HeaderPrimaryBtn"_s);
    m_newBtn->setText(tr("+ New"));
    m_newBtn->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(m_newBtn, 0, Qt::AlignVCenter);

    rightLayout->addWidget(header);

    // Pages stack
    m_pageStack = new QStackedWidget(rightContainer);

    m_pages.append(new Pages::HomePage(m_pageStack));
    m_pages.append(new Pages::StudyPage(m_pageStack));
    m_pages.append(new Pages::JobsPage(m_ctx.applications(), m_ctx.interviews(),
                                      m_ctx.exportService(), m_pageStack));
    m_pages.append(new Pages::CalendarPage(m_pageStack));
    m_pages.append(new Pages::SettingsPage(m_ctx.settings(), m_ctx.themeManager(), m_pageStack));

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

    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    });
}

void MainWindow::setupConnections() {
    connect(m_sidebar, &Sidebar::pageSelected, this, [this](int index) {
        updateHeader(index);
    });

    connect(m_sidebar, &Sidebar::collapseToggled, this, [this](bool collapsed) {
        m_ctx.settings().setSidebarCollapsed(collapsed);
    });

    connect(&m_ctx.themeManager(), &Theme::ThemeManager::themeChanged, this, [this]() {
        updateIcons();
    });
}

void MainWindow::updateHeader(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_pages.size()) return;

    m_pageStack->setCurrentIndex(pageIndex);
    auto* activePage = m_pages[pageIndex];
    m_pageTitleLabel->setText(activePage->pageTitle());
    activePage->onActivated();
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
