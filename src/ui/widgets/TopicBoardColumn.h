#pragma once

#include <array>
#include <optional>
#include <QWidget>
#include "domain/Enums.h"

class QLabel;
class QLineEdit;
class QListView;
class QMenu;
class QPoint;

namespace JobPrep::Data {
class TopicRepository;
}

namespace JobPrep::Models {
class TopicBoardProxy;
class TopicListModel;
}  // namespace JobPrep::Models

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Widgets {
class SelectionRestorer;
}

namespace JobPrep::Ui::Widgets {

/// Board column order (enum order of Domain::TopicStatus).
inline constexpr std::array<Domain::TopicStatus, 4> kBoardStatuses = {
    Domain::TopicStatus::Backlog, Domain::TopicStatus::InProgress,
    Domain::TopicStatus::Review, Domain::TopicStatus::Done};

inline int boardStatusIndex(Domain::TopicStatus status) {
    for (int i = 0; i < static_cast<int>(kBoardStatuses.size()); ++i) {
        if (kBoardStatuses[static_cast<size_t>(i)] == status) return i;
    }
    return 0;
}

/// One kanban column: header with count, quick-add line at the top (SPEC §3.2 S8),
/// and a card list with drag & drop and keyboard moves (SPEC §7.3).
class TopicBoardColumn : public QWidget {
    Q_OBJECT

public:
    TopicBoardColumn(Domain::TopicStatus status, Models::TopicListModel* model,
                     Data::TopicRepository& topics, Theme::ThemeManager& theme,
                     QWidget* parent = nullptr);
    ~TopicBoardColumn() override = default;

    Domain::TopicStatus status() const { return m_status; }
    Models::TopicBoardProxy* proxy() const { return m_proxy; }
    QListView* list() const { return m_list; }
    QLineEdit* quickAdd() const { return m_quickAdd; }

    /// Topics currently visible in this column (proxy rows).
    int itemCount() const;
    /// Row of `id` in the proxy, or -1 when hidden or unknown.
    int indexOfTopic(int id) const;
    std::optional<int> topicIdAt(int row) const;
    std::optional<int> currentTopicId() const;
    bool selectTopicId(int id);

    void focusQuickAdd();
    /// Empty `trackName` disables the quick-add with a hint (no tracks yet).
    void setQuickAddTrack(int trackId, const QString& trackName);

signals:
    /// Enter or double-click on a card.
    void topicActivated(int id);
    void deleteRequested(int id);
    /// Alt+Left/Right and the context menu: move one status step.
    void moveStatusRequested(int id, int delta);
    /// Alt+Up/Down: reorder inside this column.
    void reorderRequested(int id, int delta);
    /// Left/Right without Alt: move focus between columns.
    void columnFocusRequested(int delta);
    /// Esc on the quick-add line.
    void quickAddDismissed();
    void quickAddSubmitted(const QString& title, int trackId, Domain::TopicStatus status);

protected:
    /// Keyboard, tooltips and quick-add Esc; installed on the list and its viewport.
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void showContextMenu(const QPoint& position);
    void submitQuickAdd();

private:
    void refreshHeader();
    void openCardTooltip(const QPoint& viewportPos);

    Domain::TopicStatus m_status;
    Data::TopicRepository& m_topics;
    Theme::ThemeManager& m_theme;
    Models::TopicBoardProxy* m_proxy{nullptr};
    QListView* m_list{nullptr};
    QLineEdit* m_quickAdd{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_countLabel{nullptr};
    QMenu* m_contextMenu{nullptr};
    SelectionRestorer* m_restorer{nullptr};
    int m_quickAddTrackId{0};
};

}  // namespace JobPrep::Ui::Widgets
