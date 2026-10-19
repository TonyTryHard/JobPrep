#pragma once

#include <functional>
#include <optional>
#include "domain/Enums.h"
#include "ui/PageBase.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSplitter;
class QStackedWidget;
class QVBoxLayout;

namespace JobPrep::Data {
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Models {
class TopicListModel;
}

namespace JobPrep::Services {
class SettingsService;
}

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Widgets {
class ChipBar;
class EmptyState;
class TopicBoardColumn;
class TopicDetailPanel;
}  // namespace JobPrep::Ui::Widgets

namespace JobPrep::Ui::Pages {

/// Study preparation board (SPEC §3.2): track filter chips, a local search, four
/// kanban columns with quick-add, drag & drop and keyboard moves, plus a topic
/// detail panel with autosave. The Board|List toggle and track dialogs arrive in
/// M4 step C; their controls exist here disabled.
class StudyPage : public PageBase {
    Q_OBJECT

public:
    /// Chip id for "show every track".
    static constexpr int kAllTracks = -1;

    StudyPage(Data::TopicRepository& topics, Data::TrackRepository& tracks,
              Services::SettingsService& settings, Theme::ThemeManager& theme,
              QWidget* parent = nullptr, std::function<QDate()> todayProvider = {});
    ~StudyPage() override = default;

    void focusSearch() override;
    void triggerNew() override;
    /// Saves pending detail edits (called on window close, PageBase contract).
    void flushEdits() override;

    /// Focuses the Backlog quick-add (Part C header entry point). With no tracks it
    /// only shows a hint, because TrackDialog is not there yet.
    void newTopic();
    /// Selects `id`, clearing the track filter and the search if they hide it.
    /// Returns false for an unknown id.
    bool selectTopic(int id);
    void setDetailVisible(bool visible);
    bool isDetailVisible() const;
    Widgets::TopicDetailPanel* detailPanel() const { return m_detail; }

    void setSearchText(const QString& text);
    QString searchText() const;

    /// Index of the visible view: 0 = no topics, 1 = board, 2 = no matches (tests).
    int currentViewIndex() const;
    /// Topics visible in all four columns under the current filters.
    int visibleTopicCount() const;

protected:
    void hideEvent(QHideEvent* event) override;

private slots:
    void onTopicsChanged();
    void onTracksChanged();
    void onModelReset();
    void onChipSelected(int id);
    void onColumnActivated(int id);
    void onDeleteRequested(int id);
    void onMoveStatusRequested(int id, int delta);
    void onReorderRequested(int id, int delta);
    void onQuickAddSubmitted(const QString& title, int trackId, Domain::TopicStatus status);
    void onWriteError(const QString& what, const QString& error);

private:
    void setupUi(QVBoxLayout* layout);
    void setupToolbar(QVBoxLayout* layout);
    void setupBoard();
    void setupDetailPanel();
    void refreshChips();
    void applyFilters();
    void refreshViewState();
    void updateQuickAddTracks();
    void onCurrentChanged(const QModelIndex& current);
    void focusColumn(int fromColumn, int delta);
    void moveTopicTo(int id, Domain::TopicStatus status);
    void showMessage(const QString& text, const char* kind);
    bool matchesSearch(const QString& title, const QString& tags) const;
    Widgets::TopicBoardColumn* columnFor(Domain::TopicStatus status) const;
    Widgets::TopicBoardColumn* columnContaining(int id) const;
    std::optional<int> selectedTopicId() const;

    Data::TopicRepository& m_topics;
    Data::TrackRepository& m_tracks;
    Services::SettingsService& m_settings;
    Theme::ThemeManager& m_theme;
    std::function<QDate()> m_todayProvider;

    Models::TopicListModel* m_model{nullptr};
    Widgets::ChipBar* m_chips{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QLabel* m_messageLabel{nullptr};
    QStackedWidget* m_stack{nullptr};
    QScrollArea* m_boardScroll{nullptr};
    QSplitter* m_splitter{nullptr};
    Widgets::TopicDetailPanel* m_detail{nullptr};
    QList<Widgets::TopicBoardColumn*> m_columns;
    int m_trackFilter{kAllTracks};
    QString m_searchText;
    std::optional<int> m_selectedTopicId;
    /// True while the source model resets, so per-column restores are not mistaken
    /// for user selection changes.
    bool m_inReset{false};
    /// Quick-add selects its new card without popping the detail panel open.
    bool m_suppressDetailOpen{false};
};

}  // namespace JobPrep::Ui::Pages
