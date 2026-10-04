#include "ui/dialogs/ApplicationDialog.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QDialogButtonBox>
#include "data/ApplicationRepository.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Dialogs {

ApplicationDialog::ApplicationDialog(JobPrep::Data::ApplicationRepository& apps, Mode mode, QWidget* parent)
    : QDialog(parent), m_apps(apps), m_mode(mode) {
    setWindowTitle(mode == Create ? tr("Add Application") : tr("Edit Application"));
    setMinimumSize(800, 600);

    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    // Overview
    tabs->addTab(new QWidget(), tr("Overview"));
    // Notes
    tabs->addTab(new QWidget(), tr("Notes"));
    // Interviews
    tabs->addTab(new QWidget(), tr("Interviews"));
    // Timeline
    tabs->addTab(new QWidget(), tr("Timeline"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &ApplicationDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &ApplicationDialog::onCancel);
}

void ApplicationDialog::setApplication(const JobPrep::Domain::JobApplication& app) {
    m_app = app;
}

JobPrep::Domain::JobApplication ApplicationDialog::application() const {
    return m_app;
}

void ApplicationDialog::onSave() {
    accept();
}

void ApplicationDialog::onCancel() {
    reject();
}

}  // namespace JobPrep::Ui::Dialogs
