#include "ui/dialogs/ApplicationDialog.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include "data/ApplicationRepository.h"
#include "data/DbFormat.h"
#include "data/InterviewRepository.h"
#include "domain/EnumStrings.h"
#include "ui/theme/StatusStyle.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Dialogs {

namespace {

using JobPrep::Domain::ApplicationStatus;
using JobPrep::Domain::WorkMode;

const ApplicationStatus kStatuses[] = {
    ApplicationStatus::Wishlist, ApplicationStatus::Applied,  ApplicationStatus::HrScreen,
    ApplicationStatus::Technical, ApplicationStatus::Final,    ApplicationStatus::Offer,
    ApplicationStatus::Accepted,  ApplicationStatus::Rejected, ApplicationStatus::Withdrawn,
    ApplicationStatus::Ghosted};

const WorkMode kWorkModes[] = {WorkMode::Remote, WorkMode::Hybrid, WorkMode::Onsite};

/// Largest salary a month can realistically be; keeps the spin boxes sane.
constexpr int kMaxSalary = 100000000;
/// The date edits are optional, so they need a valid but "unset" sentinel.
const QDate kUnsetDate(2000, 1, 1);

}  // namespace

ApplicationDialog::ApplicationDialog(JobPrep::Data::ApplicationRepository& apps,
                                     JobPrep::Data::InterviewRepository& interviews,
                                     Mode mode,
                                     QWidget* parent)
    : QDialog(parent),
      m_apps(apps),
      m_interviews(interviews),
      m_mode(mode),
      m_app(Domain::JobApplication()),
      m_original(Domain::JobApplication()) {
    setObjectName(u"ApplicationDialog"_s);
    setWindowTitle(mode == Create ? tr("Add Application") : tr("Edit Application"));
    setModal(true);
    setupUi();

    m_original = m_app;
    loadFrom(m_app);
    updateDirtyState();
}

void ApplicationDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(u"ApplicationDialogTabs"_s);
    layout->addWidget(m_tabs, 1);

    setupOverviewTab();
    setupNotesTab();
    setupInterviewsTab();
    setupTimelineTab();

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(u"FieldErrorLabel"_s);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setVisible(false);
    layout->addWidget(m_messageLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_saveBtn = buttons->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    m_saveBtn->setObjectName(u"DialogSaveBtn"_s);
    m_saveBtn->setProperty("primary", true);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &ApplicationDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &ApplicationDialog::reject);
    connect(m_tabs, &QTabWidget::currentChanged, this, &ApplicationDialog::onTabChanged);
}

void ApplicationDialog::setupOverviewTab() {
    auto* page = new QWidget(m_tabs);
    auto* form = new QFormLayout(page);
    form->setSpacing(12);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_companyEdit = new QLineEdit(page);
    m_companyEdit->setObjectName(u"FieldCompany"_s);
    m_companyEdit->setPlaceholderText(tr("Acme Corp"));
    form->addRow(tr("Company *"), m_companyEdit);

    m_positionEdit = new QLineEdit(page);
    m_positionEdit->setObjectName(u"FieldPosition"_s);
    m_positionEdit->setPlaceholderText(tr("C++ Developer"));
    form->addRow(tr("Position *"), m_positionEdit);

    m_statusCombo = new QComboBox(page);
    m_statusCombo->setObjectName(u"FieldStatus"_s);
    for (const ApplicationStatus status : kStatuses) {
        m_statusCombo->addItem(Theme::StatusStyle::label(status),
                               static_cast<int>(status));
    }
    form->addRow(tr("Status"), m_statusCombo);

    m_sourceCombo = new QComboBox(page);
    m_sourceCombo->setObjectName(u"FieldSourceCombo"_s);
    m_sourceCombo->setEditable(true);
    m_sourceCombo->addItem(QString(), QString());
    m_sourceCombo->setCurrentIndex(0);
    form->addRow(tr("Source"), m_sourceCombo);

    m_workModeCombo = new QComboBox(page);
    m_workModeCombo->setObjectName(u"FieldWorkMode"_s);
    for (const WorkMode mode : kWorkModes) {
        m_workModeCombo->addItem(JobPrep::Domain::EnumStrings::toString(mode), static_cast<int>(mode));
    }
    form->addRow(tr("Work mode"), m_workModeCombo);

    m_salaryMinSpin = new QSpinBox(page);
    m_salaryMinSpin->setObjectName(u"FieldSalaryMin"_s);
    m_salaryMinSpin->setRange(0, kMaxSalary);
    m_salaryMinSpin->setSpecialValueText(tr("not set"));
    form->addRow(tr("Salary min"), m_salaryMinSpin);

    m_salaryMaxSpin = new QSpinBox(page);
    m_salaryMaxSpin->setObjectName(u"FieldSalaryMax"_s);
    m_salaryMaxSpin->setRange(0, kMaxSalary);
    m_salaryMaxSpin->setSpecialValueText(tr("not set"));
    form->addRow(tr("Salary max"), m_salaryMaxSpin);

    m_currencyEdit = new QLineEdit(page);
    m_currencyEdit->setObjectName(u"FieldCurrency"_s);
    m_currencyEdit->setMaxLength(8);
    form->addRow(tr("Currency"), m_currencyEdit);

    m_appliedDateEnabled = new QCheckBox(tr("Set"), page);
    m_appliedDateEdit = new QDateEdit(page);
    m_appliedDateEdit->setObjectName(u"FieldAppliedDate"_s);
    m_appliedDateEdit->setCalendarPopup(true);
    m_appliedDateEdit->setDate(kUnsetDate);
    m_appliedDateEdit->setEnabled(false);
    auto* appliedRow = new QWidget(page);
    auto* appliedLayout = new QHBoxLayout(appliedRow);
    appliedLayout->setContentsMargins(0, 0, 0, 0);
    appliedLayout->setSpacing(8);
    appliedLayout->addWidget(m_appliedDateEnabled);
    appliedLayout->addWidget(m_appliedDateEdit, 1);
    form->addRow(tr("Applied date"), appliedRow);

    m_nextActionEdit = new QLineEdit(page);
    m_nextActionEdit->setObjectName(u"FieldNextAction"_s);
    m_nextActionEdit->setPlaceholderText(tr("Follow up with the recruiter"));
    form->addRow(tr("Next action"), m_nextActionEdit);

    m_nextActionDateEnabled = new QCheckBox(tr("Set"), page);
    m_nextActionDateEdit = new QDateEdit(page);
    m_nextActionDateEdit->setObjectName(u"FieldNextActionDate"_s);
    m_nextActionDateEdit->setCalendarPopup(true);
    m_nextActionDateEdit->setDate(kUnsetDate);
    m_nextActionDateEdit->setEnabled(false);
    auto* nextActionRow = new QWidget(page);
    auto* nextActionLayout = new QHBoxLayout(nextActionRow);
    nextActionLayout->setContentsMargins(0, 0, 0, 0);
    nextActionLayout->setSpacing(8);
    nextActionLayout->addWidget(m_nextActionDateEnabled);
    nextActionLayout->addWidget(m_nextActionDateEdit, 1);
    form->addRow(tr("Next action date"), nextActionRow);

    m_urlEdit = new QLineEdit(page);
    m_urlEdit->setObjectName(u"FieldUrl"_s);
    m_urlEdit->setPlaceholderText(u"https://"_s);
    form->addRow(tr("URL"), m_urlEdit);

    m_resumeVersionEdit = new QLineEdit(page);
    m_resumeVersionEdit->setObjectName(u"FieldResumeVersion"_s);
    form->addRow(tr("Resume version"), m_resumeVersionEdit);

    m_locationEdit = new QLineEdit(page);
    m_locationEdit->setObjectName(u"FieldLocation"_s);
    form->addRow(tr("Location"), m_locationEdit);

    m_contactNameEdit = new QLineEdit(page);
    m_contactNameEdit->setObjectName(u"FieldContactName"_s);
    form->addRow(tr("Contact name"), m_contactNameEdit);

    m_contactEmailEdit = new QLineEdit(page);
    m_contactEmailEdit->setObjectName(u"FieldContactEmail"_s);
    form->addRow(tr("Contact email"), m_contactEmailEdit);

    m_tabs->addTab(page, tr("Overview"));

    connect(m_companyEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_positionEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_statusCombo, &QComboBox::currentIndexChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_sourceCombo, &QComboBox::currentTextChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_workModeCombo, &QComboBox::currentIndexChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_salaryMinSpin, &QSpinBox::valueChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_salaryMaxSpin, &QSpinBox::valueChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_currencyEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_appliedDateEnabled, &QCheckBox::toggled, this, &ApplicationDialog::onFormChanged);
    connect(m_appliedDateEdit, &QDateEdit::dateChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_nextActionEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_nextActionDateEnabled, &QCheckBox::toggled, this, &ApplicationDialog::onFormChanged);
    connect(m_nextActionDateEdit, &QDateEdit::dateChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_urlEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_resumeVersionEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_locationEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_contactNameEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);
    connect(m_contactEmailEdit, &QLineEdit::textChanged, this, &ApplicationDialog::onFormChanged);

    connect(m_appliedDateEnabled, &QCheckBox::toggled, m_appliedDateEdit, &QDateEdit::setEnabled);
    connect(m_nextActionDateEnabled, &QCheckBox::toggled, m_nextActionDateEdit, &QDateEdit::setEnabled);
}

void ApplicationDialog::setupNotesTab() {
    auto* page = new QWidget(m_tabs);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    m_notesEdit = new QTextEdit(page);
    m_notesEdit->setObjectName(u"FieldNotes"_s);
    m_notesEdit->setPlaceholderText(tr("Recruiter feedback, interview topics, links..."));
    layout->addWidget(m_notesEdit);
    m_tabs->addTab(page, tr("Notes"));
    connect(m_notesEdit, &QTextEdit::textChanged, this, &ApplicationDialog::onFormChanged);
}

void ApplicationDialog::setupInterviewsTab() {
    m_interviewsPage = new QWidget(m_tabs);
    auto* page = m_interviewsPage;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    m_interviewsTable = new QTableWidget(page);
    m_interviewsTable->setObjectName(u"InterviewsTable"_s);
    m_interviewsTable->setColumnCount(4);
    m_interviewsTable->setHorizontalHeaderLabels(
        {tr("When"), tr("Type"), tr("Duration"), tr("Outcome")});
    m_interviewsTable->horizontalHeader()->setStretchLastSection(true);
    m_interviewsTable->verticalHeader()->setVisible(false);
    m_interviewsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_interviewsTable->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_interviewsTable);
    m_tabs->addTab(page, tr("Interviews"));
}

void ApplicationDialog::setupTimelineTab() {
    m_timelinePage = new QWidget(m_tabs);
    auto* page = m_timelinePage;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    m_timelineTable = new QTableWidget(page);
    m_timelineTable->setObjectName(u"TimelineTable"_s);
    m_timelineTable->setColumnCount(3);
    m_timelineTable->setHorizontalHeaderLabels({tr("When"), tr("From"), tr("To")});
    m_timelineTable->horizontalHeader()->setStretchLastSection(true);
    m_timelineTable->verticalHeader()->setVisible(false);
    m_timelineTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_timelineTable->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_timelineTable);
    m_tabs->addTab(page, tr("Timeline"));
}

void ApplicationDialog::loadFrom(const JobPrep::Domain::JobApplication& app) {
    m_app = app;

    // Every widget signal is live, so the whole load runs blocked; otherwise isDirty()
    // would report an edit the user never made.
    const QSignalBlocker companyBlocker(m_companyEdit);
    const QSignalBlocker positionBlocker(m_positionEdit);
    const QSignalBlocker statusBlocker(m_statusCombo);
    const QSignalBlocker sourceBlocker(m_sourceCombo);
    const QSignalBlocker workModeBlocker(m_workModeCombo);
    const QSignalBlocker salaryMinBlocker(m_salaryMinSpin);
    const QSignalBlocker salaryMaxBlocker(m_salaryMaxSpin);
    const QSignalBlocker currencyBlocker(m_currencyEdit);
    const QSignalBlocker appliedBox(m_appliedDateEnabled);
    const QSignalBlocker appliedDate(m_appliedDateEdit);
    const QSignalBlocker nextActionBlocker(m_nextActionEdit);
    const QSignalBlocker nextActionBox(m_nextActionDateEnabled);
    const QSignalBlocker nextActionDate(m_nextActionDateEdit);
    const QSignalBlocker urlBlocker(m_urlEdit);
    const QSignalBlocker resumeBlocker(m_resumeVersionEdit);
    const QSignalBlocker locationBlocker(m_locationEdit);
    const QSignalBlocker contactNameBlocker(m_contactNameEdit);
    const QSignalBlocker contactEmailBlocker(m_contactEmailEdit);
    const QSignalBlocker notesBlocker(m_notesEdit);

    m_companyEdit->setText(app.company);
    m_positionEdit->setText(app.position);
    const int statusIndex = m_statusCombo->findData(static_cast<int>(app.status));
    m_statusCombo->setCurrentIndex(statusIndex >= 0 ? statusIndex : 0);
    m_sourceCombo->setCurrentText(app.source);
    const int workModeIndex = m_workModeCombo->findData(static_cast<int>(app.workMode));
    m_workModeCombo->setCurrentIndex(workModeIndex >= 0 ? workModeIndex : 0);
    m_salaryMinSpin->setValue(app.salaryMin.value_or(0));
    m_salaryMaxSpin->setValue(app.salaryMax.value_or(0));
    m_currencyEdit->setText(app.currency);
    m_appliedDateEnabled->setChecked(app.appliedDate.has_value());
    m_appliedDateEdit->setDate(app.appliedDate.value_or(kUnsetDate));
    m_nextActionEdit->setText(app.nextAction);
    m_nextActionDateEnabled->setChecked(app.nextActionDate.has_value());
    m_nextActionDateEdit->setDate(app.nextActionDate.value_or(kUnsetDate));
    m_urlEdit->setText(app.url);
    m_resumeVersionEdit->setText(app.resumeVersion);
    m_locationEdit->setText(app.location);
    m_contactNameEdit->setText(app.contactName);
    m_contactEmailEdit->setText(app.contactEmail);
    m_notesEdit->setPlainText(app.notes);
    m_appliedDateEdit->setEnabled(app.appliedDate.has_value());
    m_nextActionDateEdit->setEnabled(app.nextActionDate.has_value());

    reloadReadOnlyTabs();
}

void ApplicationDialog::reloadReadOnlyTabs() {
    // A new application has no id yet, so there is nothing to list.
    const bool hasRow = m_app.id != 0;
    m_tabs->setTabEnabled(m_tabs->indexOf(m_interviewsPage), hasRow);
    m_tabs->setTabEnabled(m_tabs->indexOf(m_timelinePage), hasRow);

    m_interviewsTable->setRowCount(0);
    m_timelineTable->setRowCount(0);
    if (!hasRow) return;

    const auto interviews = m_interviews.byApplication(m_app.id);
    m_interviewsTable->setRowCount(interviews.size());
    for (int row = 0; row < interviews.size(); ++row) {
        const auto& interview = interviews.at(row);
        m_interviewsTable->setItem(row, 0, new QTableWidgetItem(
            JobPrep::Data::DbFormat::toText(interview.startAt.date())));
        m_interviewsTable->setItem(row, 1, new QTableWidgetItem(
            JobPrep::Domain::EnumStrings::toString(interview.type)));
        m_interviewsTable->setItem(row, 2, new QTableWidgetItem(
            tr("%1 min").arg(interview.durationMin)));
        m_interviewsTable->setItem(row, 3, new QTableWidgetItem(
            JobPrep::Domain::EnumStrings::toString(interview.outcome)));
    }

    const auto history = m_apps.statusHistory(m_app.id);
    m_timelineTable->setRowCount(history.size());
    for (int row = 0; row < history.size(); ++row) {
        const auto& change = history.at(row);
        m_timelineTable->setItem(row, 0, new QTableWidgetItem(
            JobPrep::Data::DbFormat::toText(change.changedAt.date())));
        m_timelineTable->setItem(row, 1, new QTableWidgetItem(
            change.fromStatus.has_value() ? Theme::StatusStyle::label(*change.fromStatus)
                                          : tr("—")));
        m_timelineTable->setItem(row, 2, new QTableWidgetItem(
            Theme::StatusStyle::label(change.toStatus)));
    }
}

void ApplicationDialog::setApplication(const JobPrep::Domain::JobApplication& app) {
    m_original = app;
    loadFrom(app);
    updateDirtyState();
}

JobPrep::Domain::JobApplication ApplicationDialog::application() const {
    auto app = m_original;
    app.company = m_companyEdit->text().trimmed();
    app.position = m_positionEdit->text().trimmed();
    app.status = static_cast<ApplicationStatus>(m_statusCombo->currentData().toInt());
    app.source = m_sourceCombo->currentText().trimmed();
    app.workMode = static_cast<WorkMode>(m_workModeCombo->currentData().toInt());
    app.salaryMin = m_salaryMinSpin->value() > 0
                        ? std::optional<int>(m_salaryMinSpin->value())
                        : std::nullopt;
    app.salaryMax = m_salaryMaxSpin->value() > 0
                        ? std::optional<int>(m_salaryMaxSpin->value())
                        : std::nullopt;
    app.currency = m_currencyEdit->text().trimmed();
    app.appliedDate = m_appliedDateEnabled->isChecked()
                          ? std::optional<QDate>(m_appliedDateEdit->date())
                          : std::nullopt;
    app.nextAction = m_nextActionEdit->text().trimmed();
    app.nextActionDate = m_nextActionDateEnabled->isChecked()
                             ? std::optional<QDate>(m_nextActionDateEdit->date())
                             : std::nullopt;
    app.url = m_urlEdit->text().trimmed();
    app.resumeVersion = m_resumeVersionEdit->text().trimmed();
    app.location = m_locationEdit->text().trimmed();
    app.contactName = m_contactNameEdit->text().trimmed();
    app.contactEmail = m_contactEmailEdit->text().trimmed();
    app.notes = m_notesEdit->toPlainText();
    return app;
}

QString ApplicationDialog::validationMessage() const {
    // The label text, not its visibility: the dialog itself may not be shown yet.
    return m_messageLabel->text();
}

bool ApplicationDialog::isDirty() const {
    return m_dirty;
}

void ApplicationDialog::onFormChanged() {
    updateDirtyState();
}

void ApplicationDialog::onTabChanged(int index) {
    if (m_tabs->widget(index) != m_interviewsPage && m_tabs->widget(index) != m_timelinePage) {
        return;
    }
    // The tables show saved rows, so they must not suggest unsaved edits.
    reloadReadOnlyTabs();
}

void ApplicationDialog::updateDirtyState() {
    m_dirty = m_mode == Create ? !isEmptyForm() : application() != m_original;
}

bool ApplicationDialog::validateForm() {
    QStringList errors;
    QStringList warnings;
    if (m_companyEdit->text().trimmed().isEmpty()) errors.append(tr("Company is required."));
    if (m_positionEdit->text().trimmed().isEmpty()) errors.append(tr("Position is required."));
    if (m_salaryMinSpin->value() > 0 && m_salaryMaxSpin->value() > 0 &&
        m_salaryMinSpin->value() > m_salaryMaxSpin->value()) {
        errors.append(tr("Salary min cannot be greater than salary max."));
    }

    const QString email = m_contactEmailEdit->text().trimmed();
    // A malformed address only warns: it must not block saving the application.
    if (!email.isEmpty() && !email.contains(u'@')) {
        warnings.append(tr("Warning: the contact e-mail does not look like an address."));
    }

    showMessage((errors + warnings).join(u' '));
    return errors.isEmpty();
}

void ApplicationDialog::showMessage(const QString& message) {
    m_messageLabel->setText(message);
    m_messageLabel->setVisible(!message.isEmpty());
}

bool ApplicationDialog::isEmptyForm() const {
    return application().company.isEmpty() && application().position.isEmpty() &&
           application().notes.isEmpty();
}

bool ApplicationDialog::confirmDiscard() {
    if (!m_dirty) return true;
    const auto answer = QMessageBox::question(
        this, tr("Discard changes?"),
        tr("This application has unsaved changes. Discard them?"),
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

void ApplicationDialog::onSave() {
    if (!validateForm()) {
        m_tabs->setCurrentIndex(0);
        return;
    }
    auto app = application();
    const bool ok = m_mode == Create ? m_apps.insert(app) : m_apps.update(app);
    if (!ok) {
        showMessage(tr("Could not save: %1").arg(m_apps.lastError()));
        return;
    }
    m_app = app;
    m_original = app;
    m_dirty = false;
    accept();
}

void ApplicationDialog::reject() {
    if (confirmDiscard()) QDialog::reject();
}

void ApplicationDialog::closeEvent(QCloseEvent* event) {
    if (!confirmDiscard()) {
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

}  // namespace JobPrep::Ui::Dialogs