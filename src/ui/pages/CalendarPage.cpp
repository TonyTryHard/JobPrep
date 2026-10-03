#include "ui/pages/CalendarPage.h"
#include <QVBoxLayout>
#include "ui/widgets/EmptyState.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

CalendarPage::CalendarPage(QWidget* parent)
    : PageBase(tr("Calendar"), parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* emptyState = new Widgets::EmptyState(
        u"calendar"_s,
        tr("No Scheduled Events"),
        tr("View your upcoming technical interviews, screening calls, and study deadlines on a monthly calendar."),
        QString(),
        this);

    layout->addWidget(emptyState);
}

}  // namespace JobPrep::Ui::Pages
