#include "ui/dialogs/FollowUpDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
#include "data/ApplicationRepository.h"
#include "data/DbFormat.h"
#include "domain/Structs.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Dialogs {

FollowUpDialog::FollowUpDialog(JobPrep::Data::ApplicationRepository& apps, QDate date,
                               QWidget* parent)
    : QDialog(parent), m_apps(apps), m_date(date) {
    setObjectName(u"FollowUpDialog"_s);
    setWindowTitle(tr("Add Follow-up"));
    setModal(true);
    setupUi();
    populateApplications();
}

int FollowUpDialog::applicationId() const {
    return m_appCombo->currentData().toInt();
}

QString FollowUpDialog::actionText() const {
    return m_actionEdit->text().trimmed();
}

QString FollowUpDialog::validationMessage() const {
    if (m_appCombo->count() == 0) {
        return tr("No applications available. Add an application first.");
    }
    if (actionText().isEmpty()) {
        return tr("Describe the follow-up action.");
    }
    return QString();
}

void FollowUpDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto* form = new QFormLayout;
    form->setSpacing(8);

    m_appCombo = new QComboBox(this);
    m_appCombo->setObjectName(u"FollowUpApplicationCombo"_s);
    m_appCombo->setToolTip(tr("The application to follow up on"));
    form->addRow(tr("&Application *"), m_appCombo);

    m_actionEdit = new QLineEdit(this);
    m_actionEdit->setObjectName(u"FollowUpActionEdit"_s);
    m_actionEdit->setPlaceholderText(tr("Remind the recruiter, send a thank-you note..."));
    m_actionEdit->setToolTip(tr("What you will do on this day"));
    m_actionEdit->setClearButtonEnabled(true);
    form->addRow(tr("&Action *"), m_actionEdit);

    auto* dateLabel = new QLabel(JobPrep::Data::DbFormat::toText(m_date), this);
    dateLabel->setObjectName(u"FollowUpDateLabel"_s);
    dateLabel->setToolTip(tr("Planned date, taken from the selected day"));
    form->addRow(tr("Date"), dateLabel);

    layout->addLayout(form);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(u"FieldErrorLabel"_s);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setVisible(false);
    layout->addWidget(m_messageLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_saveBtn = buttons->addButton(tr("&Save"), QDialogButtonBox::AcceptRole);
    m_saveBtn->setObjectName(u"DialogSaveBtn"_s);
    m_saveBtn->setProperty("primary", true);
    m_saveBtn->setDefault(true);
    m_saveBtn->setAutoDefault(true);
    m_saveBtn->setToolTip(tr("Schedule the follow-up (Ctrl+S)"));
    buttons->button(QDialogButtonBox::Cancel)->setToolTip(tr("Cancel (Esc)"));
    layout->addWidget(buttons);

    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &FollowUpDialog::accept);

    QWidget::setTabOrder(m_appCombo, m_actionEdit);
    QWidget::setTabOrder(m_actionEdit, m_saveBtn);
    QWidget::setTabOrder(m_saveBtn, buttons->button(QDialogButtonBox::Cancel));

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, &FollowUpDialog::accept);
}

void FollowUpDialog::populateApplications() {
    // Follow-ups need an open process, so closed applications are not offered.
    const auto apps = m_apps.all();
    m_appCombo->clear();
    for (const auto& app : apps) {
        const bool closed = app.status == Domain::ApplicationStatus::Rejected ||
                            app.status == Domain::ApplicationStatus::Withdrawn ||
                            app.status == Domain::ApplicationStatus::Ghosted ||
                            app.status == Domain::ApplicationStatus::Accepted;
        if (closed) continue;
        m_appCombo->addItem(u"%1 \u2013 %2"_s.arg(app.company, app.position), app.id);
    }
}

bool FollowUpDialog::confirmOverwrite(int applicationId) {
    const auto stored = m_apps.byId(applicationId);
    if (!stored || !stored->nextActionDate.has_value()) return true;
    const auto answer = QMessageBox::question(
        this, tr("Replace follow-up"),
        tr("%1 already has a follow-up planned for %2. Replace it with %3?")
            .arg(stored->company, JobPrep::Data::DbFormat::toText(*stored->nextActionDate),
                 JobPrep::Data::DbFormat::toText(m_date)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void FollowUpDialog::accept() {
    const QString problem = validationMessage();
    if (m_messageLabel->isVisible()) m_messageLabel->setVisible(false);
    if (!problem.isEmpty()) {
        m_messageLabel->setText(problem);
        m_messageLabel->setVisible(true);
        m_actionEdit->setFocus();
        return;
    }
    if (!confirmOverwrite(applicationId())) return;
    m_apps.setNextAction(applicationId(), actionText(), QDate(m_date));
    QDialog::accept();
}

}  // namespace JobPrep::Ui::Dialogs