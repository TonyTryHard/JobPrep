#include "ui/pages/JobsPage.h"
#include <QVBoxLayout>
#include "ui/widgets/EmptyState.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

JobsPage::JobsPage(QWidget* parent)
    : PageBase(tr("Job Applications"), parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* emptyState = new Widgets::EmptyState(
        u"jobs"_s,
        tr("No Applications Yet"),
        tr("Track job openings, manage interview stages, and follow up with recruiters."),
        QString(),
        this);

    layout->addWidget(emptyState);
}

}  // namespace JobPrep::Ui::Pages
