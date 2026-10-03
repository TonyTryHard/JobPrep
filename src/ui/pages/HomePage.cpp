#include "ui/pages/HomePage.h"
#include <QVBoxLayout>
#include "ui/widgets/EmptyState.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

HomePage::HomePage(QWidget* parent)
    : PageBase(tr("Dashboard"), parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* emptyState = new Widgets::EmptyState(
        u"home"_s,
        tr("No Activity Yet"),
        tr("Welcome to JobPrep! Your daily study targets, active applications, and upcoming interviews will appear here."),
        QString(),
        this);

    layout->addWidget(emptyState);
}

}  // namespace JobPrep::Ui::Pages
