#include "ui/pages/HomePage.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "models/AgendaListModel.h"
#include "ui/FormatHelper.h"
#include "ui/delegates/AgendaItemDelegate.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/FunnelWidget.h"
#include "ui/widgets/GoalProgressBar.h"
#include "ui/widgets/ProgressRing.h"
#include "ui/widgets/StatCard.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

HomePage::HomePage(DashboardController& controller,
                   Theme::ThemeManager& theme,
                   QWidget* parent)
    : PageBase(tr("Dashboard"), parent),
      m_controller(controller),
      m_theme(theme) {
    setupUi();

    connect(&m_controller, &DashboardController::snapshotChanged, this,
            &HomePage::applySnapshot);

    applySnapshot(m_controller.snapshot());
}

void HomePage::onActivated() {
    m_controller.requestRefresh();
}

void HomePage::triggerNew() {
    emit addApplicationRequested();
}

QFrame* HomePage::createCard(const QString& title, QWidget* content) {
    auto* card = new QFrame(this);
    card->setProperty("card", true);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(12);

    auto* titleLabel = new QLabel(title, card);
    QFont titleFont = font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAccessibleName(title);

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(content, 1);

    return card;
}

void HomePage::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    rootLayout->addWidget(m_stack);

    // 1. Dashboard content in scroll area
    auto* scrollArea = new QScrollArea(m_stack);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_dashboardContent = new QWidget(scrollArea);
    auto* mainLayout = new QVBoxLayout(m_dashboardContent);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // 1a. Top KPI cards (4 across)
    auto* kpiLayout = new QHBoxLayout();
    kpiLayout->setSpacing(12);

    m_cardApplied = new Widgets::StatCard(m_theme, m_dashboardContent);
    m_cardApplied->setTitle(tr("Applied this week"));
    m_cardApplied->setValue(u"0"_s);

    m_cardActive = new Widgets::StatCard(m_theme, m_dashboardContent);
    m_cardActive->setTitle(tr("Active processes"));
    m_cardActive->setValue(u"0"_s);

    m_cardInterviews = new Widgets::StatCard(m_theme, m_dashboardContent);
    m_cardInterviews->setTitle(tr("Upcoming interviews"));
    m_cardInterviews->setSubtitle(tr("Next 7 days"));
    m_cardInterviews->setValue(u"0"_s);

    m_cardStreak = new Widgets::StatCard(m_theme, m_dashboardContent);
    m_cardStreak->setTitle(tr("Study streak"));
    m_cardStreak->setValue(tr("0-day streak"));

    kpiLayout->addWidget(m_cardApplied, 1);
    kpiLayout->addWidget(m_cardActive, 1);
    kpiLayout->addWidget(m_cardInterviews, 1);
    kpiLayout->addWidget(m_cardStreak, 1);
    mainLayout->addLayout(kpiLayout);

    // 1b. Two-column layout below
    auto* columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(16);

    // Left column: Today list + Upcoming interviews
    auto* leftCol = new QVBoxLayout();
    leftCol->setSpacing(16);

    // Today container
    auto* todayContainer = new QWidget(m_dashboardContent);
    auto* todayLayout = new QVBoxLayout(todayContainer);
    todayLayout->setContentsMargins(0, 0, 0, 0);

    m_todayModel = new Models::AgendaListModel(this);
    m_todayDelegate = new Delegates::AgendaItemDelegate(m_theme, this);

    m_todayView = new QListView(todayContainer);
    m_todayView->setUniformItemSizes(true);
    m_todayView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_todayView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_todayView->setItemDelegate(m_todayDelegate);
    m_todayView->setModel(m_todayModel);
    m_todayView->setFocusPolicy(Qt::StrongFocus);
    m_todayView->setMinimumHeight(QFontMetrics(font()).height() * 7);

    m_todayEmptyState = new Widgets::EmptyState(
        u"calendar"_s, tr("Nothing planned for today"),
        tr("Log a study session or follow up on an application."),
        QString(), todayContainer);
    m_todayEmptyState->setObjectName(u"TodayEmptyState"_s);

    connect(m_todayView, &QListView::activated, this, &HomePage::onTodayItemActivated);
    connect(m_todayView, &QListView::doubleClicked, this, &HomePage::onTodayItemActivated);

    todayLayout->addWidget(m_todayView);
    todayLayout->addWidget(m_todayEmptyState);

    auto* todayCard = createCard(tr("Today"), todayContainer);
    leftCol->addWidget(todayCard, 1);

    // Upcoming container
    auto* upcomingContainer = new QWidget(m_dashboardContent);
    auto* upcomingLayout = new QVBoxLayout(upcomingContainer);
    upcomingLayout->setContentsMargins(0, 0, 0, 0);

    m_upcomingModel = new Models::AgendaListModel(this);
    m_upcomingDelegate = new Delegates::AgendaItemDelegate(m_theme, this);

    m_upcomingView = new QListView(upcomingContainer);
    m_upcomingView->setUniformItemSizes(true);
    m_upcomingView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_upcomingView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_upcomingView->setItemDelegate(m_upcomingDelegate);
    m_upcomingView->setModel(m_upcomingModel);
    m_upcomingView->setFocusPolicy(Qt::StrongFocus);
    m_upcomingView->setMinimumHeight(QFontMetrics(font()).height() * 6);

    m_upcomingEmptyState = new Widgets::EmptyState(
        u"calendar"_s, tr("No upcoming interviews"),
        tr("Schedule your next interview stage or screening."),
        tr("+ Schedule interview"), upcomingContainer);
    m_upcomingEmptyState->setObjectName(u"UpcomingEmptyState"_s);

    connect(m_upcomingView, &QListView::activated, this, &HomePage::onUpcomingItemActivated);
    connect(m_upcomingView, &QListView::doubleClicked, this, &HomePage::onUpcomingItemActivated);
    connect(m_upcomingEmptyState, &Widgets::EmptyState::actionClicked, this,
            &HomePage::newInterviewRequested);

    upcomingLayout->addWidget(m_upcomingView);
    upcomingLayout->addWidget(m_upcomingEmptyState);

    QWidget::setTabOrder(m_todayView, m_upcomingView);

    auto* upcomingCard = createCard(tr("Upcoming interviews"), upcomingContainer);
    leftCol->addWidget(upcomingCard, 1);

    columnsLayout->addLayout(leftCol, 1);

    // Right column: Study progress + Hiring funnel
    auto* rightCol = new QVBoxLayout();
    rightCol->setSpacing(16);

    // Study progress container
    auto* studyContainer = new QWidget(m_dashboardContent);
    auto* studyLayout = new QVBoxLayout(studyContainer);
    studyLayout->setContentsMargins(0, 0, 0, 0);
    studyLayout->setSpacing(12);

    m_progressRings = new Widgets::ProgressRing(m_theme, studyContainer);
    m_goalBar = new Widgets::GoalProgressBar(m_theme, studyContainer);

    studyLayout->addWidget(m_progressRings);
    studyLayout->addWidget(m_goalBar);

    auto* studyCard = createCard(tr("Study progress"), studyContainer);
    rightCol->addWidget(studyCard, 1);

    // Hiring funnel container
    auto* funnelContainer = new QWidget(m_dashboardContent);
    auto* funnelLayout = new QVBoxLayout(funnelContainer);
    funnelLayout->setContentsMargins(0, 0, 0, 0);

    m_funnelWidget = new Widgets::FunnelWidget(m_theme, funnelContainer);
    funnelLayout->addWidget(m_funnelWidget);

    auto* funnelCard = createCard(tr("Hiring funnel"), funnelContainer);
    rightCol->addWidget(funnelCard, 1);

    columnsLayout->addLayout(rightCol, 1);
    mainLayout->addLayout(columnsLayout, 1);

    scrollArea->setWidget(m_dashboardContent);
    m_stack->addWidget(scrollArea);

    // 2. Empty state (D6)
    m_emptyState = new Widgets::EmptyState(
        u"home"_s,
        tr("No Activity Yet"),
        tr("Welcome to JobPrep! Your daily study targets, active applications, and upcoming interviews will appear here."),
        tr("+ Add application"),
        m_stack);
    m_emptyState->setSecondaryActionText(tr("+ Add topic"));

    connect(m_emptyState, &Widgets::EmptyState::actionClicked, this,
            &HomePage::addApplicationRequested);
    connect(m_emptyState, &Widgets::EmptyState::secondaryActionClicked, this,
            &HomePage::addTopicRequested);

    m_stack->addWidget(m_emptyState);
}

void HomePage::applySnapshot(const DashboardSnapshot& snapshot) {
    m_progressRings->setTracks(snapshot.stats.trackProgress);
    m_goalBar->setGoal(snapshot.stats.weekMinutes, snapshot.weeklyGoalMinutes);

    if (snapshot.isDataEmpty) {
        m_stack->setCurrentWidget(m_emptyState);
        return;
    }
    m_stack->setCurrentIndex(0);

    // KPIs
    m_cardApplied->setValue(QString::number(snapshot.stats.appliedThisWeek));
    m_cardActive->setValue(QString::number(snapshot.stats.activeProcesses));
    m_cardInterviews->setValue(QString::number(snapshot.stats.interviewsNext7Days));
    m_cardStreak->setValue(FormatHelper::formatStreak(snapshot.stats.studyStreakDays));

    // Today list
    m_todayModel->setItems(snapshot.todayItems);
    const bool hasToday = !snapshot.todayItems.isEmpty();
    m_todayView->setVisible(hasToday);
    m_todayEmptyState->setVisible(!hasToday);

    // Upcoming interviews
    m_upcomingModel->setItems(snapshot.upcomingInterviews);
    const bool hasUpcoming = !snapshot.upcomingInterviews.isEmpty();
    m_upcomingView->setVisible(hasUpcoming);
    m_upcomingEmptyState->setVisible(!hasUpcoming);

    // Study progress
    m_progressRings->setTracks(snapshot.stats.trackProgress);
    m_goalBar->setGoal(snapshot.stats.weekMinutes, snapshot.weeklyGoalMinutes);

    // Hiring funnel
    m_funnelWidget->setFunnel(snapshot.stats.funnel, snapshot.stats.interviewRate);
}

void HomePage::onTodayItemActivated(const QModelIndex& index) {
    if (!index.isValid()) return;
    const auto kind = static_cast<Domain::AgendaKind>(
        index.data(Models::AgendaListModel::KindRole).toInt());
    const int refId = index.data(Models::AgendaListModel::IdRole).toInt();

    switch (kind) {
        case Domain::AgendaKind::Interview:
            emit interviewActivated(refId);
            break;
        case Domain::AgendaKind::FollowUp:
            emit applicationActivated(refId);
            break;
        case Domain::AgendaKind::StudyTarget:
            emit topicActivated(refId);
            break;
    }
}

void HomePage::onUpcomingItemActivated(const QModelIndex& index) {
    if (!index.isValid()) return;
    const int refId = index.data(Models::AgendaListModel::IdRole).toInt();
    emit interviewActivated(refId);
}

}  // namespace JobPrep::Ui::Pages
