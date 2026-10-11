#pragma once

#include <QDate>
#include "ui/PageBase.h"

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class TopicRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {
class AgendaService;
class SettingsService;
}  // namespace JobPrep::Services

namespace JobPrep::Ui::Theme {
class ThemeManager;
}  // namespace JobPrep::Ui::Theme

namespace JobPrep::Ui::Pages {

/// Calendar and agenda page. Part B provides the create-interview entry points; the
/// month grid and agenda panel arrive in Part C, which reuses the same references.
class CalendarPage : public PageBase {
    Q_OBJECT

public:
    CalendarPage(JobPrep::Data::InterviewRepository& interviews,
                 JobPrep::Data::ApplicationRepository& apps,
                 JobPrep::Data::TopicRepository& topics,
                 JobPrep::Services::AgendaService& agenda,
                 JobPrep::Services::SettingsService& settings,
                 JobPrep::Ui::Theme::ThemeManager& theme,
                 QWidget* parent = nullptr);
    ~CalendarPage() override = default;

    /// The day new interviews default to: the selected day, or today before Part C.
    QDate selectedDay() const;

    /// True when `date` has at least one agenda item, from the same service the
    /// month grid will read. Part B seam for the end-to-end smoke test.
    bool hasItemsOn(QDate date) const;

    void triggerNew() override;

public slots:
    /// Opens a new-interview dialog. An invalid `date` falls back to today.
    void newInterview(QDate date = QDate());

private:
    JobPrep::Data::InterviewRepository& m_interviews;
    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::TopicRepository& m_topics;
    JobPrep::Services::AgendaService& m_agenda;
    JobPrep::Services::SettingsService& m_settings;
    JobPrep::Ui::Theme::ThemeManager& m_theme;
};

}  // namespace JobPrep::Ui::Pages
