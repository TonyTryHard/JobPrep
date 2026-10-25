#include "ui/widgets/TopicListView.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QStyleOptionViewItem>
#include "models/TopicListModel.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

TopicListView::TopicListView(Theme::ThemeManager& theme, QWidget* parent)
    : QListView(parent), m_theme(theme) {
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

bool TopicListView::event(QEvent* event) {
    switch (event->type()) {
        case QEvent::DragEnter:
            dragEnterEvent(static_cast<QDragEnterEvent*>(event));
            return event->isAccepted();
        case QEvent::DragMove:
            dragMoveEvent(static_cast<QDragMoveEvent*>(event));
            return event->isAccepted();
        case QEvent::DragLeave:
            dragLeaveEvent(static_cast<QDragLeaveEvent*>(event));
            return event->isAccepted();
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent*>(event));
            return event->isAccepted();
        default:
            break;
    }
    return QListView::event(event);
}

bool TopicListView::viewportEvent(QEvent* event) {
    switch (event->type()) {
        case QEvent::DragEnter:
            dragEnterEvent(static_cast<QDragEnterEvent*>(event));
            return event->isAccepted();
        case QEvent::DragMove:
            dragMoveEvent(static_cast<QDragMoveEvent*>(event));
            return event->isAccepted();
        case QEvent::DragLeave:
            dragLeaveEvent(static_cast<QDragLeaveEvent*>(event));
            return event->isAccepted();
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent*>(event));
            return event->isAccepted();
        default:
            break;
    }
    return QListView::viewportEvent(event);
}

void TopicListView::startDrag(Qt::DropActions supportedActions) {
    if (m_isDragging) return;
    const QModelIndex current = currentIndex();
    if (!current.isValid() || !model()) return;

    QMimeData* mime = model()->mimeData({current});
    if (!mime) return;

    m_isDragging = true;
    auto* drag = new QDrag(this);
    drag->setMimeData(mime);

    const QRect rect = visualRect(current);
    if (!rect.isEmpty()) {
        QPixmap pixmap(rect.size());
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        option.rect = QRect(QPoint(0, 0), rect.size());
        if (auto* delegate = itemDelegateForIndex(current)) {
            delegate->paint(&painter, option, current);
        }
        painter.end();
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    }

    drag->exec(supportedActions, Qt::MoveAction);
    m_isDragging = false;
}

void TopicListView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat(u"application/x-jobprep-topic-id"_s)) {
        event->acceptProposedAction();
        m_dragOver = true;
        updateDropIndicator(event->position().toPoint());
        emit dragOverChanged(true);
        viewport()->update();
        update();
    } else {
        QListView::dragEnterEvent(event);
    }
}

void TopicListView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat(u"application/x-jobprep-topic-id"_s)) {
        event->acceptProposedAction();
        const bool wasOver = m_dragOver;
        m_dragOver = true;
        updateDropIndicator(event->position().toPoint());
        if (!wasOver) {
            emit dragOverChanged(true);
            update();
        }
        viewport()->update();
    } else {
        QListView::dragMoveEvent(event);
    }
}

void TopicListView::dragLeaveEvent(QDragLeaveEvent* event) {
    m_dragOver = false;
    m_dropIndicatorRect = QRect();
    emit dragOverChanged(false);
    viewport()->update();
    update();
    QListView::dragLeaveEvent(event);
}

void TopicListView::dropEvent(QDropEvent* event) {
    m_dragOver = false;
    m_dropIndicatorRect = QRect();
    emit dragOverChanged(false);
    viewport()->update();
    update();
    QListView::dropEvent(event);
}

void TopicListView::updateDropIndicator(const QPoint& pos) {
    if (!model() || model()->rowCount() == 0) {
        m_dropIndicatorRect = QRect();
        return;
    }

    const int rows = model()->rowCount();
    int lineY = 0;
    const QModelIndex idx = indexAt(pos);
    if (!idx.isValid()) {
        const QRect lastRect = visualRect(model()->index(rows - 1, 0));
        if (pos.y() > lastRect.center().y()) {
            lineY = lastRect.bottom() + 4;
        } else {
            const QRect firstRect = visualRect(model()->index(0, 0));
            lineY = firstRect.top() - 4;
        }
    } else {
        const QRect itemRect = visualRect(idx);
        if (pos.y() < itemRect.center().y()) {
            lineY = itemRect.top() - 4;
        } else {
            lineY = itemRect.bottom() + 4;
        }
    }

    constexpr int kMargin = 8;
    const int w = qMax(40, viewport()->width() - 2 * kMargin);
    m_dropIndicatorRect = QRect(kMargin, lineY - 1, w, 3);
}

void TopicListView::paintEvent(QPaintEvent* event) {
    QListView::paintEvent(event);

    if (m_dragOver) {
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing);
        const auto& tokens = m_theme.currentTokens();

        if (model() && model()->rowCount() == 0) {
            QPen dashPen(tokens.accent);
            dashPen.setWidth(2);
            dashPen.setStyle(Qt::DashLine);
            p.setPen(dashPen);
            QColor fill = tokens.accent;
            fill.setAlpha(25);
            p.setBrush(fill);
            p.drawRoundedRect(viewport()->rect().adjusted(4, 4, -4, -4), 8, 8);
        } else if (!m_dropIndicatorRect.isNull()) {
            p.setPen(Qt::NoPen);
            p.setBrush(tokens.accent);
            p.drawRoundedRect(m_dropIndicatorRect, 1.5, 1.5);
            p.drawEllipse(QPointF(m_dropIndicatorRect.left(), m_dropIndicatorRect.center().y()), 3.0, 3.0);
            p.drawEllipse(QPointF(m_dropIndicatorRect.right(), m_dropIndicatorRect.center().y()), 3.0, 3.0);
        }
    }
}

}  // namespace JobPrep::Ui::Widgets
