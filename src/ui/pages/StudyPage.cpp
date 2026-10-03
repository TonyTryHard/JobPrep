#include "ui/pages/StudyPage.h"
#include <QVBoxLayout>
#include "ui/widgets/EmptyState.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

StudyPage::StudyPage(QWidget* parent)
    : PageBase(tr("Study Tracks"), parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* emptyState = new Widgets::EmptyState(
        u"study"_s,
        tr("No Study Topics"),
        tr("Organize your C++ and English interview topics into kanban columns and log study sessions."),
        QString(),
        this);

    layout->addWidget(emptyState);
}

}  // namespace JobPrep::Ui::Pages
