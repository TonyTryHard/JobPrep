#pragma once

#include <functional>
#include <optional>
#include <QDate>
#include <QDialog>
#include "domain/Structs.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;

namespace JobPrep::Data {
class SessionRepository;
class TopicRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Ui::Dialogs {

/// Log a study session (SPEC §3.2 S4, §5 `study_sessions`).
/// Allows logging minutes, date, note, and optional topic link.
class SessionDialog : public QDialog {
    Q_OBJECT

public:
    explicit SessionDialog(Data::SessionRepository& sessions,
                           Data::TopicRepository& topics,
                           QWidget* parent = nullptr,
                           std::function<QDate()> todayProvider = {});
    ~SessionDialog() override = default;

    /// Preselects a topic, or (No topic) when nullopt or invalid.
    void setTopicId(std::optional<int> topicId);
    void setDate(QDate date);
    void setMinutes(int minutes);
    void setNote(const QString& note);

    Domain::StudySession session() const;
    QString validationMessage() const;

private slots:
    void onSave();
    void onFormChanged();

private:
    void setupUi();
    void populateTopics();
    bool validateForm();

    Data::SessionRepository& m_sessions;
    Data::TopicRepository& m_topics;
    std::function<QDate()> m_todayProvider;

    QDateEdit* m_dateEdit{nullptr};
    QSpinBox* m_minutesSpin{nullptr};
    QComboBox* m_topicCombo{nullptr};
    QTextEdit* m_noteEdit{nullptr};
    QLabel* m_messageLabel{nullptr};
    QPushButton* m_saveBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};

    Domain::StudySession m_session;
};

}  // namespace JobPrep::Ui::Dialogs
