#pragma once

#include <QDialog>
#include <optional>
#include "domain/Structs.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;
class QTextEdit;

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
}

namespace JobPrep::Ui::Dialogs {

/// Create or edit one job application (SPEC §3.3 A4). The Overview tab holds every
/// field of `applications`, Notes the free text, Interviews and Timeline are read-only
/// views of the related rows.
class ApplicationDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, Edit };

    ApplicationDialog(JobPrep::Data::ApplicationRepository& apps,
                      JobPrep::Data::InterviewRepository& interviews,
                      Mode mode,
                      QWidget* parent = nullptr);
    ~ApplicationDialog() override = default;

    /// Fills every widget from `application` and marks the form clean. JobsPage calls
    /// this after construction because an Edit dialog needs its id to load the
    /// Interviews and Timeline tabs.
    void setApplication(const JobPrep::Domain::JobApplication& application);

    /// The record being edited; in Create mode it is the blank draft.
    JobPrep::Domain::JobApplication application() const;

    /// Validation problem shown under the form, empty when the form is valid.
    QString validationMessage() const;

    /// True when the form differs from what was loaded.
    bool isDirty() const;

public slots:
    /// Asks before discarding, so an accidental Cancel cannot lose edits.
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSave();
    void onFormChanged();
    void onTabChanged(int index);

private:
    void setupUi();
    void setupOverviewTab();
    void setupNotesTab();
    void setupInterviewsTab();
    void setupTimelineTab();
    void loadFrom(const JobPrep::Domain::JobApplication& app);
    void reloadReadOnlyTabs();
    void updateDirtyState();
    /// Returns false and shows inline messages when company, position or the salary
    /// range are wrong. A malformed contact e-mail only warns.
    bool validateForm();
    bool confirmDiscard();
    bool isEmptyForm() const;
    void showMessage(const QString& message);

    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::InterviewRepository& m_interviews;
    Mode m_mode;
    JobPrep::Domain::JobApplication m_app;
    JobPrep::Domain::JobApplication m_original;
    bool m_dirty{false};

    QTabWidget* m_tabs{nullptr};
    QLineEdit* m_companyEdit{nullptr};
    QLineEdit* m_positionEdit{nullptr};
    QComboBox* m_statusCombo{nullptr};
    QComboBox* m_sourceCombo{nullptr};
    QComboBox* m_workModeCombo{nullptr};
    QSpinBox* m_salaryMinSpin{nullptr};
    QSpinBox* m_salaryMaxSpin{nullptr};
    QLineEdit* m_currencyEdit{nullptr};
    QDateEdit* m_appliedDateEdit{nullptr};
    QCheckBox* m_appliedDateEnabled{nullptr};
    QLineEdit* m_nextActionEdit{nullptr};
    QDateEdit* m_nextActionDateEdit{nullptr};
    QCheckBox* m_nextActionDateEnabled{nullptr};
    QLineEdit* m_urlEdit{nullptr};
    QLineEdit* m_resumeVersionEdit{nullptr};
    QLineEdit* m_locationEdit{nullptr};
    QLineEdit* m_contactNameEdit{nullptr};
    QLineEdit* m_contactEmailEdit{nullptr};
    QLabel* m_messageLabel{nullptr};
    QTextEdit* m_notesEdit{nullptr};
    QWidget* m_interviewsPage{nullptr};
    QWidget* m_timelinePage{nullptr};
    QTableWidget* m_interviewsTable{nullptr};
    QTableWidget* m_timelineTable{nullptr};
    QPushButton* m_saveBtn{nullptr};
};

}  // namespace JobPrep::Ui::Dialogs