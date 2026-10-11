#include "ui/dialogs/InterviewDialog.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimeEdit>
#include <QVBoxLayout>
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "ui/theme/StatusStyle.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Dialogs {

namespace {

using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::InterviewOutcome;
using JobPrep::Domain::InterviewType;
using JobPrep::Domain::JobApplication;

const InterviewType kTypes[] = {InterviewType::Hr,         InterviewType::Technical,
                                InterviewType::LiveCoding, InterviewType::SystemDesign,
                                InterviewType::Behavioral, InterviewType::Final,
                                InterviewType::Other};

const InterviewOutcome kOutcomes[] = {InterviewOutcome::Pending, InterviewOutcome::Passed,
                                      InterviewOutcome::Failed, InterviewOutcome::Cancelled};

/// Statuses offered in the application combo: everything that is not closed.
QList<ApplicationStatus> openStatuses() {
    return {ApplicationStatus::Wishlist, ApplicationStatus::Applied,
            ApplicationStatus::HrScreen, ApplicationStatus::Technical,
            ApplicationStatus::Final,    ApplicationStatus::Offer};
}

QString applicationLabel(const JobApplication& app) {
    if (app.position.isEmpty()) return app.company;
    return u"%1 — %2"_s.arg(app.company, app.position);
}

/// A sensible default: the next full hour, so a fresh interview does not start "now".
QDateTime defaultStart() {
    const QDateTime now = QDateTime::currentDateTime();
    return QDateTime(now.date(), QTime(now.time().hour(), 0));
}

}  // namespace

InterviewDialog::InterviewDialog(JobPrep::Data::ApplicationRepository& apps,
                                 JobPrep::Data::InterviewRepository& interviews,
                                 Mode mode,
                                 QWidget* parent)
    : QDialog(parent),
      m_apps(apps),
      m_interviews(interviews),
      m_mode(mode),
      m_interview(Domain::Interview()),
      m_original(Domain::Interview()) {
    m_interview.startAt = defaultStart();
    setObjectName(u"InterviewDialog"_s);
    setWindowTitle(mode == Create ? tr("Add Interview") : tr("Edit Interview"));
    setModal(true);
    setupUi();

    m_original = m_interview;
    loadFrom(m_interview);
}

void InterviewDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* form = new QFormLayout();
    form->setSpacing(12);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->addLayout(form);

    m_appCombo = new QComboBox(this);
    m_appCombo->setObjectName(u"InterviewApplicationCombo"_s);
    m_appCombo->setToolTip(tr("The application this interview belongs to"));
    form->addRow(tr("&Application *"), m_appCombo);

    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setObjectName(u"InterviewDateEdit"_s);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setToolTip(tr("Interview date"));
    m_timeEdit = new QTimeEdit(this);
    m_timeEdit->setObjectName(u"InterviewTimeEdit"_s);
    m_timeEdit->setDisplayFormat(u"HH:mm"_s);
    m_timeEdit->setToolTip(tr("Start time"));
    auto* whenRow = new QWidget(this);
    auto* whenLayout = new QHBoxLayout(whenRow);
    whenLayout->setContentsMargins(0, 0, 0, 0);
    whenLayout->setSpacing(8);
    whenLayout->addWidget(m_dateEdit, 1);
    whenLayout->addWidget(m_timeEdit, 0);
    form->addRow(tr("&When *"), whenRow);

    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setObjectName(u"InterviewDurationSpin"_s);
    m_durationSpin->setRange(0, 1440);
    m_durationSpin->setValue(60);
    m_durationSpin->setSuffix(tr(" min"));
    m_durationSpin->setToolTip(tr("Expected duration in minutes"));
    form->addRow(tr("&Duration"), m_durationSpin);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->setObjectName(u"InterviewTypeCombo"_s);
    m_typeCombo->setToolTip(tr("Which round this is"));
    for (const InterviewType type : kTypes) {
        m_typeCombo->addItem(Theme::StatusStyle::label(type), static_cast<int>(type));
    }
    form->addRow(tr("&Type"), m_typeCombo);

    m_outcomeCombo = new QComboBox(this);
    m_outcomeCombo->setObjectName(u"InterviewOutcomeCombo"_s);
    m_outcomeCombo->setToolTip(tr("Result so far"));
    for (const InterviewOutcome outcome : kOutcomes) {
        m_outcomeCombo->addItem(Theme::StatusStyle::label(outcome), static_cast<int>(outcome));
    }
    form->addRow(tr("&Outcome"), m_outcomeCombo);

    m_placeEdit = new QLineEdit(this);
    m_placeEdit->setObjectName(u"InterviewPlaceEdit"_s);
    m_placeEdit->setPlaceholderText(tr("Zoom, on-site address..."));
    m_placeEdit->setToolTip(tr("Where the interview takes place"));
    form->addRow(tr("&Place"), m_placeEdit);

    m_interviewerEdit = new QLineEdit(this);
    m_interviewerEdit->setObjectName(u"InterviewInterviewerEdit"_s);
    m_interviewerEdit->setToolTip(tr("Who you will talk to"));
    form->addRow(tr("&Interviewer"), m_interviewerEdit);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setObjectName(u"InterviewNotesEdit"_s);
    m_notesEdit->setPlaceholderText(tr("Preparation notes, questions to ask..."));
    m_notesEdit->setToolTip(tr("Preparation notes"));
    form->addRow(tr("&Notes"), m_notesEdit);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(u"FieldErrorLabel"_s);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setVisible(false);
    layout->addWidget(m_messageLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Cancel)->setToolTip(tr("Discard changes (Esc)"));
    m_saveBtn = buttons->addButton(tr("&Save"), QDialogButtonBox::AcceptRole);
    m_saveBtn->setObjectName(u"DialogSaveBtn"_s);
    m_saveBtn->setProperty("primary", true);
    m_saveBtn->setDefault(true);
    m_saveBtn->setAutoDefault(true);
    m_saveBtn->setToolTip(tr("Save the interview (Enter)"));
    if (m_mode == Edit) {
        m_deleteBtn = buttons->addButton(tr("&Delete"), QDialogButtonBox::DestructiveRole);
        m_deleteBtn->setObjectName(u"DialogDeleteBtn"_s);
        m_deleteBtn->setToolTip(tr("Delete this interview"));
    }
    layout->addWidget(buttons);

    populateApplications();

    connect(buttons, &QDialogButtonBox::accepted, this, &InterviewDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &InterviewDialog::reject);
    if (m_deleteBtn) {
        connect(m_deleteBtn, &QPushButton::clicked, this, &InterviewDialog::onDelete);
    }
    connect(m_appCombo, &QComboBox::currentIndexChanged, this, &InterviewDialog::onFormChanged);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &InterviewDialog::onFormChanged);
    connect(m_timeEdit, &QTimeEdit::timeChanged, this, &InterviewDialog::onFormChanged);
    connect(m_durationSpin, &QSpinBox::valueChanged, this, &InterviewDialog::onFormChanged);
    connect(m_typeCombo, &QComboBox::currentIndexChanged, this, &InterviewDialog::onFormChanged);
    connect(m_outcomeCombo, &QComboBox::currentIndexChanged, this, &InterviewDialog::onFormChanged);
    connect(m_placeEdit, &QLineEdit::textChanged, this, &InterviewDialog::onFormChanged);
    connect(m_interviewerEdit, &QLineEdit::textChanged, this, &InterviewDialog::onFormChanged);
    connect(m_notesEdit, &QTextEdit::textChanged, this, &InterviewDialog::onFormChanged);
}

void InterviewDialog::populateApplications() {
    const QSignalBlocker blocker(m_appCombo);
    m_appCombo->clear();
    for (const auto& app : m_apps.byStatuses(openStatuses())) {
        m_appCombo->addItem(applicationLabel(app), app.id);
    }
}

void InterviewDialog::ensureApplicationOffered(int applicationId) {
    if (applicationId <= 0) return;
    if (m_appCombo->findData(applicationId) >= 0) return;
    const auto stored = m_apps.byId(applicationId);
    if (!stored) return;
    m_appCombo->addItem(applicationLabel(*stored), stored->id);
}

void InterviewDialog::setInterview(const JobPrep::Domain::Interview& interview) {
    m_original = interview;
    loadFrom(interview);
    m_dirty = false;
}

void InterviewDialog::setApplicationId(int applicationId) {
    ensureApplicationOffered(applicationId);
    const int index = m_appCombo->findData(applicationId);
    if (index >= 0) m_appCombo->setCurrentIndex(index);
}

void InterviewDialog::setStartDate(QDate date) {
    if (date.isValid()) m_dateEdit->setDate(date);
}

void InterviewDialog::loadFrom(const JobPrep::Domain::Interview& interview) {
    m_interview = interview;

    const QSignalBlocker appBlocker(m_appCombo);
    const QSignalBlocker dateBlocker(m_dateEdit);
    const QSignalBlocker timeBlocker(m_timeEdit);
    const QSignalBlocker durationBlocker(m_durationSpin);
    const QSignalBlocker typeBlocker(m_typeCombo);
    const QSignalBlocker outcomeBlocker(m_outcomeCombo);
    const QSignalBlocker placeBlocker(m_placeEdit);
    const QSignalBlocker interviewerBlocker(m_interviewerEdit);
    const QSignalBlocker notesBlocker(m_notesEdit);

    ensureApplicationOffered(interview.applicationId);
    const int appIndex = m_appCombo->findData(interview.applicationId);
    m_appCombo->setCurrentIndex(appIndex >= 0 ? appIndex : 0);
    m_dateEdit->setDate(interview.startAt.date());
    m_timeEdit->setTime(interview.startAt.time());
    m_durationSpin->setValue(interview.durationMin);
    const int typeIndex = m_typeCombo->findData(static_cast<int>(interview.type));
    m_typeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
    const int outcomeIndex = m_outcomeCombo->findData(static_cast<int>(interview.outcome));
    m_outcomeCombo->setCurrentIndex(outcomeIndex >= 0 ? outcomeIndex : 0);
    m_placeEdit->setText(interview.place);
    m_interviewerEdit->setText(interview.interviewer);
    m_notesEdit->setPlainText(interview.notes);

    m_dirty = false;
}

JobPrep::Domain::Interview InterviewDialog::interview() const {
    auto interview = m_interview;
    interview.applicationId = m_appCombo->currentData().toInt();
    interview.startAt = QDateTime(m_dateEdit->date(), m_timeEdit->time());
    interview.durationMin = m_durationSpin->value();
    interview.type = static_cast<InterviewType>(m_typeCombo->currentData().toInt());
    interview.outcome = static_cast<InterviewOutcome>(m_outcomeCombo->currentData().toInt());
    interview.place = m_placeEdit->text().trimmed();
    interview.interviewer = m_interviewerEdit->text().trimmed();
    interview.notes = m_notesEdit->toPlainText();
    return interview;
}

QString InterviewDialog::validationMessage() const {
    return m_messageLabel->text();
}

bool InterviewDialog::isDirty() const {
    return m_dirty;
}

void InterviewDialog::onFormChanged() {
    m_dirty = true;
}

bool InterviewDialog::validateForm() {
    QStringList errors;
    if (m_appCombo->currentIndex() < 0 || m_appCombo->currentData().toInt() <= 0) {
        errors.append(tr("An application is required."));
    }
    if (m_durationSpin->value() < 1) {
        errors.append(tr("Duration must be at least 1 minute."));
    }
    showMessage(errors.join(u' '));
    return errors.isEmpty();
}

void InterviewDialog::showMessage(const QString& message) {
    m_messageLabel->setText(message);
    m_messageLabel->setVisible(!message.isEmpty());
}

bool InterviewDialog::confirmDiscard() {
    if (!m_dirty) return true;
    const auto answer = QMessageBox::question(
        this, tr("Discard changes?"),
        tr("This interview has unsaved changes. Discard them?"),
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

void InterviewDialog::onSave() {
    if (!validateForm()) return;
    auto interview = this->interview();
    const bool ok =
        m_mode == Create ? m_interviews.insert(interview) : m_interviews.update(interview);
    if (!ok) {
        showMessage(tr("Could not save: %1").arg(m_interviews.lastError()));
        return;
    }
    m_interview = interview;
    m_original = interview;
    m_dirty = false;
    accept();
}

void InterviewDialog::onDelete() {
    if (m_mode != Edit || m_interview.id == 0) return;
    const auto answer =
        QMessageBox::question(this, tr("Delete interview"),
                              tr("Delete this interview? This cannot be undone."),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    if (!m_interviews.remove(m_interview.id)) {
        showMessage(tr("Could not delete: %1").arg(m_interviews.lastError()));
        return;
    }
    // Deleting is not a discard: closing must not ask again.
    m_dirty = false;
    accept();
}

void InterviewDialog::reject() {
    if (confirmDiscard()) QDialog::reject();
}

void InterviewDialog::closeEvent(QCloseEvent* event) {
    if (!confirmDiscard()) {
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

}  // namespace JobPrep::Ui::Dialogs
