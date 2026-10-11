#include "ui/pages/CalendarPage.h"

#include <QVBoxLayout>
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/TopicRepository.h"
#include "services/AgendaService.h"
#include "services/SettingsService.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/theme/ThemeManager.h"
#include "ui/widgets/EmptyState.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

CalendarPage::CalendarPage(JobPrep::Data::InterviewRepository& interviews,
                           JobPrep::Data::ApplicationRepository& apps,
                           JobPrep::Data::TopicRepository& topics,
                           JobPrep::Services::AgendaService& agenda,
                           JobPrep::Services::SettingsService& settings,
                           JobPrep::Ui::Theme::ThemeManager& theme,
                           QWidget* parent)
    : PageBase(tr("Calendar"), parent),
      m_interviews(interviews),
      m_apps(apps),
      m_topics(topics),
      m_agenda(agenda),
      m_settings(settings),
      m_theme(theme) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* emptyState = new Widgets::EmptyState(
        u"calendar"_s, tr("No Scheduled Events"),
        tr("View your upcoming technical interviews, screening calls, and study deadlines on a "
           "monthly calendar."),
        tr("+ Interview"), this);
    connect(emptyState, &Widgets::EmptyState::actionClicked, this,
            [this]() { newInterview(QDate()); });

    layout->addWidget(emptyState);
}

QDate CalendarPage::selectedDay() const {
    // Part C adds day selection to the month grid; before that, new interviews start today.
    return QDate::currentDate();
}

bool CalendarPage::hasItemsOn(QDate date) const {
    if (!date.isValid()) return false;
    return !m_agenda.items(date, date).isEmpty();
}

void CalendarPage::newInterview(QDate date) {
    const QDate day = date.isValid() ? date : QDate::currentDate();
    Ui::Dialogs::InterviewDialog dialog(m_apps, m_interviews,
                                        Ui::Dialogs::InterviewDialog::Create, this);
    dialog.setStartDate(day);
    dialog.exec();
}

void CalendarPage::triggerNew() {
    newInterview(selectedDay());
}

}  // namespace JobPrep::Ui::Pages
