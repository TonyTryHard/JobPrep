#pragma once

#include <QListView>
#include <QRect>

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Widgets {

/// Kanban card list view providing drag-and-drop feedback:
/// - Custom accent-coloured insertion line between cards
/// - Full-column drag-over highlight and empty-column drop indicator
/// - Drag pixmap generation of the dragged topic card
class TopicListView : public QListView {
    Q_OBJECT

public:
    explicit TopicListView(Theme::ThemeManager& theme, QWidget* parent = nullptr);
    ~TopicListView() override = default;

    bool isDragOver() const { return m_dragOver; }
    QRect dropIndicatorRect() const { return m_dropIndicatorRect; }

signals:
    void dragOverChanged(bool isOver);

protected:
    bool event(QEvent* event) override;
    bool viewportEvent(QEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateDropIndicator(const QPoint& pos);

    Theme::ThemeManager& m_theme;
    bool m_dragOver{false};
    bool m_isDragging{false};
    QRect m_dropIndicatorRect;
};

}  // namespace JobPrep::Ui::Widgets
