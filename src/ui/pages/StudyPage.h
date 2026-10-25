#pragma once

#include <functional>
#include <optional>
#include "domain/Enums.h"
#include "ui/PageBase.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSortFilterProxyModel;
class QSplitter;
class QStackedWidget;
class QTableView;
class QVBoxLayout;

namespace JobPrep::Data {
class SessionRepository;
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
class SelectionRestorer;
class TopicBoardColumn;
class TopicDetailPanel;
}  // namespace JobPrep::Ui::Widgets

namespace JobPrep::Ui::Pages {

/// Study preparation board and list view (SPEC §3.2):
/// - Track filter chips, "+ Track" and "Manage tracks..." dialog
/// - View toggle [Board | List] persisted in settings
/// - Kanban board: 4 columns, quick-add, drag-and-drop, keyboard reordering
/// - Flat sortable list view with Topic status badge and progress bar delegates
/// - Collapsible topic detail panel with autosave and "Log time" SessionDialog
class StudyPage : public PageBase {
    Q_OBJECT

public:
    /// Chip id for "show every track".
    static constexpr int kAllTracks = -1;

    StudyPage(Data::TopicRepository& topics,
              Data::TrackRepository& tracks,
              Data::SessionRepository& sessions,
              Services::SettingsService& settings,
              Theme::ThemeManager& theme,
              QWidget* parent = nullptr,
              std::function<QDate()> todayProvider = {});
    ~StudyPage() override = default;

    void focusSearch() override;
    void triggerNew() override;
    /// Saves pending detail edits (called on window close, PageBase contract).
    void flushEdits() override;

    /// Focuses Backlog quick-add (switches to board if in list). If no tracks, opens TrackDialog.
    void newTopic();
    /// Selects `id`, clearing the track filter and the search if they hide it.
    /// Returns false for an unknown id.
    bool selectTopic(int id);
    void setDetailVisible(bool visible);
    bool isDetailVisible() const;
    Widgets::TopicDetailPanel* detailPanel() const { return m_detail; }

    void setSearchText(const QString& text);
    QString searchText() const;

    /// "board" or "list"
    void setViewMode(const QString& mode);
    QString viewMode() const;

    /// Index of the visible view stack: 0 = no topics, 1 = board/list, 2 = no matches, 3 = no tracks.
    int currentViewIndex() const;
    /// Topics visible under the current filters.
    int visibleTopicCount() const;

    std::optional<int> selectedTopicId() const;

    QTableView* tableView() const { return m_tableView; }

public slots:
    void openTrackDialog();
    void openSessionDialog(std::optional<int> topicId = std::nullopt);

protected:
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

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
    void onTableRowActivated(const QModelIndex& index);

private:
    void setupUi(QVBoxLayout* layout);
    void setupToolbar(QVBoxLayout* layout);
    void setupSplitterAndViews();
    void setupBoardView();
    void setupTableView();
    void setupDetailPanel();
    void refreshChips();
    void applyFilters();
    void refreshViewState();
    void updateQuickAddTracks();
    void onColumnCurrentChanged(Widgets::TopicBoardColumn* activeColumn, const QModelIndex& current);
    void onColumnSelectionChanged(Widgets::TopicBoardColumn* activeColumn);
    void onTableSelectionChanged();
    void focusColumn(int fromColumn, int delta);
    void moveTopicTo(int id, Domain::TopicStatus status);
    void showMessage(const QString& text, const char* kind);
    bool matchesSearch(const QString& title, const QString& tags) const;
    Widgets::TopicBoardColumn* columnFor(Domain::TopicStatus status) const;
    Widgets::TopicBoardColumn* columnContaining(int id) const;

    Data::TopicRepository& m_topics;
    Data::TrackRepository& m_tracks;
    Data::SessionRepository& m_sessions;
    Services::SettingsService& m_settings;
    Theme::ThemeManager& m_theme;
    std::function<QDate()> m_todayProvider;

    Models::TopicListModel* m_model{nullptr};
    Widgets::ChipBar* m_chips{nullptr};
    QPushButton* m_addTrackBtn{nullptr};
    QPushButton* m_manageTracksBtn{nullptr};
    QPushButton* m_boardBtn{nullptr};
    QPushButton* m_listBtn{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QLabel* m_messageLabel{nullptr};
    QStackedWidget* m_stack{nullptr};
    QSplitter* m_splitter{nullptr};
    QStackedWidget* m_viewStack{nullptr};
    QScrollArea* m_boardScroll{nullptr};
    QTableView* m_tableView{nullptr};
    QSortFilterProxyModel* m_tableProxy{nullptr};
    Widgets::SelectionRestorer* m_tableRestorer{nullptr};
    Widgets::TopicDetailPanel* m_detail{nullptr};
    QList<Widgets::TopicBoardColumn*> m_columns;
    int m_trackFilter{kAllTracks};
    QString m_searchText;
    std::optional<int> m_selectedTopicId;
    bool m_inReset{false};
    bool m_suppressDetailOpen{false};
};

}  // namespace JobPrep::Ui::Pages
