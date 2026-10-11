#pragma once

#include <QDate>
#include <QDialog>
#include "domain/Structs.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QTimeEdit;

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
}

namespace JobPrep::Ui::Dialogs {

/// Create, edit or delete one interview (SPEC §3.4 C5, §5 `interviews`). The dialog
/// saves to the repository as soon as it is accepted, so the Interviews tab of
/// `ApplicationDialog` can call it without touching the application's dirty state.
class InterviewDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, Edit };

    InterviewDialog(JobPrep::Data::ApplicationRepository& apps,
                    JobPrep::Data::InterviewRepository& interviews,
                    Mode mode,
                    QWidget* parent = nullptr);
    ~InterviewDialog() override = default;

    /// Fills every widget from `interview` and marks the form clean.
    void setInterview(const JobPrep::Domain::Interview& interview);

    /// Preselects an application; adds it to the combo when it is closed or absent.
    void setApplicationId(int applicationId);

    /// Preselects the start date, keeping the current time. A null date is ignored.
    void setStartDate(QDate date);

    /// The record being edited; in Create mode it is the current form contents.
    JobPrep::Domain::Interview interview() const;

    /// Validation problem shown under the form, empty when the form is valid.
    QString validationMessage() const;

    /// True when the form was changed since it was loaded.
    bool isDirty() const;

    /// Database id of the interview being edited, 0 in Create mode.
    int interviewId() const { return m_interview.id; }

public slots:
    /// Asks before discarding, so an accidental Cancel cannot lose edits.
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSave();
    void onDelete();
    void onFormChanged();

private:
    void setupUi();
    void populateApplications();
    /// Ensures `applicationId` is offered in the combo, loading it even when closed.
    void ensureApplicationOffered(int applicationId);
    void loadFrom(const JobPrep::Domain::Interview& interview);
    bool validateForm();
    bool confirmDiscard();
    void showMessage(const QString& message);

    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::InterviewRepository& m_interviews;
    Mode m_mode;
    JobPrep::Domain::Interview m_interview;
    JobPrep::Domain::Interview m_original;
    bool m_dirty{false};

    QComboBox* m_appCombo{nullptr};
    QDateEdit* m_dateEdit{nullptr};
    QTimeEdit* m_timeEdit{nullptr};
    QSpinBox* m_durationSpin{nullptr};
    QComboBox* m_typeCombo{nullptr};
    QComboBox* m_outcomeCombo{nullptr};
    QLineEdit* m_placeEdit{nullptr};
    QLineEdit* m_interviewerEdit{nullptr};
    QTextEdit* m_notesEdit{nullptr};
    QLabel* m_messageLabel{nullptr};
    QPushButton* m_saveBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
};

}  // namespace JobPrep::Ui::Dialogs
