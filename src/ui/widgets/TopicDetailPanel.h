#pragma once

#include <functional>
#include <optional>
#include <QList>
#include <QWidget>
#include "domain/Structs.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QScrollArea;
class QTableWidget;
class QTextEdit;
class QUrl;

namespace JobPrep::Data {
class TopicRepository;
}

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Widgets {

/// Right-hand editor for the selected study topic (SPEC §3.2 S2/S3/S4/S7): title,
/// track, status, priority, target date, notes, resources, tags, checklist and the
/// disabled "Log time" button. Edits autosave per amendment B5; an empty title is
/// never saved. Resource links open only with http/https.
class TopicDetailPanel : public QWidget {
    Q_OBJECT

public:
    /// Test seam: the app passes an opener around QDesktopServices::openUrl.
    using OpenUrlOpener = std::function<bool(const QUrl&)>;

    explicit TopicDetailPanel(Data::TopicRepository& topics, Theme::ThemeManager& theme,
                              QWidget* parent = nullptr, OpenUrlOpener opener = {});
    ~TopicDetailPanel() override = default;

    std::optional<int> currentTopicId() const { return m_topicId; }
    bool isDirty() const { return m_dirty; }

    /// Rebuilds the track combo (a track was added, renamed or deleted).
    void setTracks(const QList<Domain::Track>& tracks);
    /// Loads `topicId`, flushing pending edits of a previous topic. False when unknown.
    bool loadFrom(int topicId);
    /// Saves pending edits (autosave path). True when nothing needed saving.
    bool flush();
    /// Drops the current topic without saving; used when the topic is gone.
    void clearTopic();
    /// Re-reads the current topic from the repository, keeping the user's edits.
    void refreshFromRepository();
    /// True while an editor inside this panel holds focus (autosave must not clobber it).
    bool hasFocusInEditor() const;

    /// "Log time" stays disabled until M4 step C provides SessionDialog.
    void setSessionLogEnabled(bool enabled);
    /// Focuses the first editor (title), used when Enter opens the detail panel (B1).
    void focusFirstEditor();

signals:
    /// User clicked "Log time" (C1): opens SessionDialog with this topic preselected.
    void logTimeRequested(int topicId);
    /// A repository write failed (SPEC §8, B10): the page shows a message and reloads.
    void writeFailed(const QString& error);
    /// Esc inside the panel (B1): the page saves and collapses it.
    void closeRequested();

protected:
    /// Focus-out autosave and checklist keyboard handling (Alt+Up/Down, Delete).
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void markDirty();
    void onChecklistItemChanged(QListWidgetItem* item);
    void addChecklistItem();
    void deleteChecklistItem();
    void moveChecklistItem(int delta);
    void addResource();
    void openResource();
    void deleteResource();
    void clearTargetDate();

private:
    bool saveNow();
    void loadEditors(const Domain::Topic& topic);
    void fillChecklist(int topicId);
    void fillResources(const QString& json);
    QString resourcesJson() const;
    void persistChecklistOrder();
    void showMessage(const QString& text, const char* kind);
    void hideMessage();
    void updateResourceButtons();
    void updateDateControls();
    void setFormEnabled(bool enabled);

    Data::TopicRepository& m_topics;
    Theme::ThemeManager& m_theme;
    OpenUrlOpener m_opener;
    std::optional<int> m_topicId;
    bool m_dirty{false};
    bool m_loading{false};
    /// True while this panel drives a repository write, so the resulting model reset
    /// does not refill the form under the user (the write itself just completed).
    bool m_internalWrite{false};
    bool m_hasTargetDate{false};

    QWidget* m_form{nullptr};
    QLabel* m_headingLabel{nullptr};
    QLabel* m_messageLabel{nullptr};
    QLineEdit* m_titleEdit{nullptr};
    QComboBox* m_trackCombo{nullptr};
    QComboBox* m_statusCombo{nullptr};
    QComboBox* m_priorityCombo{nullptr};
    QDateEdit* m_dateEdit{nullptr};
    QPushButton* m_dateBtn{nullptr};
    QLineEdit* m_tagsEdit{nullptr};
    QTextEdit* m_notesEdit{nullptr};
    QListWidget* m_checkList{nullptr};
    QLineEdit* m_checkAddEdit{nullptr};
    QTableWidget* m_resourcesTable{nullptr};
    QLineEdit* m_resourceTitleEdit{nullptr};
    QLineEdit* m_resourceUrlEdit{nullptr};
    QPushButton* m_resourceOpenBtn{nullptr};
    QPushButton* m_resourceDeleteBtn{nullptr};
    QPushButton* m_logTimeBtn{nullptr};
    QPushButton* m_closeBtn{nullptr};
    QScrollArea* m_scroll{nullptr};
};

}  // namespace JobPrep::Ui::Widgets
