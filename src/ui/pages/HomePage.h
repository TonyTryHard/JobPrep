#pragma once

#include <memory>
#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QStackedWidget>
#include "ui/PageBase.h"
#include "ui/DashboardController.h"

namespace JobPrep::Models {
class AgendaListModel;
}  // namespace JobPrep::Models

namespace JobPrep::Ui::Delegates {
class AgendaItemDelegate;
}  // namespace JobPrep::Ui::Delegates

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::Ui::Widgets {
class EmptyState;
class FunnelWidget;
class GoalProgressBar;
class ProgressRing;
class StatCard;
}  // namespace JobPrep::Ui::Widgets

namespace JobPrep::Ui::Pages {

/// Dashboard page displaying KPI stat cards, Today list, Upcoming interviews,
/// study progress rings, weekly goal bar, and hiring funnel (SPEC §7.2, Amendment B1-B8).
class HomePage : public PageBase {
    Q_OBJECT

public:
    explicit HomePage(DashboardController& controller,
                      Theme::ThemeManager& theme,
                      QWidget* parent = nullptr);
    ~HomePage() override = default;

    void onActivated() override;
    void triggerNew() override;

signals:
    void addApplicationRequested();
    void addTopicRequested();
    void newInterviewRequested();
    void interviewActivated(int id);
    void applicationActivated(int id);
    void topicActivated(int id);

private slots:
    void applySnapshot(const JobPrep::Ui::DashboardSnapshot& snapshot);
    void onTodayItemActivated(const QModelIndex& index);
    void onUpcomingItemActivated(const QModelIndex& index);

private:
    void setupUi();
    QFrame* createCard(const QString& title, QWidget* content);

    DashboardController& m_controller;
    Theme::ThemeManager& m_theme;

    QStackedWidget* m_stack{nullptr};
    Widgets::EmptyState* m_emptyState{nullptr};
    QWidget* m_dashboardContent{nullptr};

    // 4 StatCards across the top
    Widgets::StatCard* m_cardApplied{nullptr};
    Widgets::StatCard* m_cardActive{nullptr};
    Widgets::StatCard* m_cardInterviews{nullptr};
    Widgets::StatCard* m_cardStreak{nullptr};

    // Today agenda list
    Models::AgendaListModel* m_todayModel{nullptr};
    Delegates::AgendaItemDelegate* m_todayDelegate{nullptr};
    QListView* m_todayView{nullptr};
    Widgets::EmptyState* m_todayEmptyState{nullptr};

    // Upcoming interviews list
    Models::AgendaListModel* m_upcomingModel{nullptr};
    Delegates::AgendaItemDelegate* m_upcomingDelegate{nullptr};
    QListView* m_upcomingView{nullptr};
    Widgets::EmptyState* m_upcomingEmptyState{nullptr};

    // Study progress card
    Widgets::ProgressRing* m_progressRings{nullptr};
    Widgets::GoalProgressBar* m_goalBar{nullptr};

    // Hiring funnel card
    Widgets::FunnelWidget* m_funnelWidget{nullptr};
};

}  // namespace JobPrep::Ui::Pages
