#pragma once

#include <QDate>
#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace JobPrep::Data {
class ApplicationRepository;
}

namespace JobPrep::Ui::Dialogs {

/// Creates a follow-up for one application: picks the application and the action text,
/// then sets its `next_action_date` to the selected calendar day (SPEC §3.4 C4). If that
/// application already has a follow-up, saving asks before replacing it.
class FollowUpDialog : public QDialog {
    Q_OBJECT

public:
    FollowUpDialog(JobPrep::Data::ApplicationRepository& apps, QDate date,
                   QWidget* parent = nullptr);
    ~FollowUpDialog() override = default;

    int applicationId() const;
    QString actionText() const;
    QDate date() const { return m_date; }

    /// Validation problem shown under the form, empty when the form is valid.
    QString validationMessage() const;

public slots:
    /// Validates and confirms an overwrite before accepting.
    void accept() override;

private:
    void setupUi();
    void populateApplications();
    bool confirmOverwrite(int applicationId);

    JobPrep::Data::ApplicationRepository& m_apps;
    QDate m_date;

    QComboBox* m_appCombo{nullptr};
    QLineEdit* m_actionEdit{nullptr};
    QLabel* m_messageLabel{nullptr};
    QPushButton* m_saveBtn{nullptr};
};

}  // namespace JobPrep::Ui::Dialogs