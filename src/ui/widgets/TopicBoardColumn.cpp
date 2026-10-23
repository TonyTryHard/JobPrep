#include "ui/widgets/TopicBoardColumn.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include "data/TopicRepository.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"
#include "ui/delegates/TopicCardDelegate.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/SelectionRestorer.h"
#include "ui/widgets/TopicListView.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

namespace {

/// A filled circle in the status colour, used as the column header's dot.
QIcon statusDotIcon(const QColor& color) {
    constexpr int kSize = 10;
    QPixmap pixmap(kSize, kSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRectF(1.0, 1.0, kSize - 2.0, kSize - 2.0));
    return QIcon(pixmap);
}

}  // namespace

TopicBoardColumn::TopicBoardColumn(Domain::TopicStatus status, Models::TopicListModel* model,
                                   Data::TopicRepository& topics, Theme::ThemeManager& theme,
                                   QWidget* parent)
    : QWidget(parent), m_status(status), m_topics(topics), m_theme(theme) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_quickAdd = new QLineEdit(this);
    m_quickAdd->setObjectName(u"StudyColumnQuickAdd"_s);
    m_quickAdd->setClearButtonEnabled(false);
    m_quickAdd->installEventFilter(this);
    layout->addWidget(m_quickAdd);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(2, 0, 2, 0);
    headerLayout->setSpacing(6);

    m_titleLabel = new QLabel(header);
    m_titleLabel->setObjectName(u"StudyColumnTitle"_s);
    m_titleLabel->setPixmap(statusDotIcon(Theme::StatusStyle::color(status)).pixmap(10, 10));
    m_titleLabel->setText(Theme::StatusStyle::label(status));
    m_titleLabel->setAlignment(Qt::AlignVCenter);

    m_countLabel = new QLabel(header);
    m_countLabel->setObjectName(u"StudyColumnCount"_s);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_countLabel);
    layout->addWidget(header);

    m_list = new TopicListView(m_theme, this);
    connect(m_list, &TopicListView::dragOverChanged, this, [this] { update(); });
    m_list->setObjectName(u"StudyColumnList"_s);
    m_list->setViewMode(QListView::ListMode);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setMovement(QListView::Static);
    m_list->setDragEnabled(true);
    m_list->setAcceptDrops(true);
    m_list->setDropIndicatorShown(true);
    m_list->setDragDropMode(QAbstractItemView::DragDrop);
    m_list->setDefaultDropAction(Qt::MoveAction);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMouseTracking(true);
    m_list->setItemDelegate(new Delegates::TopicCardDelegate(m_theme, m_list));
    m_list->installEventFilter(this);
    m_list->viewport()->installEventFilter(this);
    layout->addWidget(m_list, 1);

    m_proxy = new Models::TopicBoardProxy(status, topics, this);
    m_proxy->setSourceModel(model);
    m_list->setModel(m_proxy);
    m_restorer = new SelectionRestorer(m_list, Models::TopicListModel::IdRole, this);

    connect(m_list, &QListView::customContextMenuRequested, this,
            &TopicBoardColumn::showContextMenu);
    connect(m_list, &QListView::activated, this, [this](const QModelIndex& index) {
        if (index.isValid()) emit topicActivated(index.data(Models::TopicListModel::IdRole).toInt());
    });
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this,
            [this] { refreshHeader(); });
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, [this] { refreshHeader(); });
    connect(m_proxy, &QAbstractItemModel::modelReset, this, [this] { refreshHeader(); });

    connect(m_quickAdd, &QLineEdit::returnPressed, this, &TopicBoardColumn::submitQuickAdd);

    m_contextMenu = new QMenu(this);
    m_contextMenu->setObjectName(u"StudyContextMenu"_s);
    auto* moveMenu = m_contextMenu->addMenu(tr("Move to"));
    moveMenu->setObjectName(u"StudyMoveToMenu"_s);
    for (int i = 0; i < static_cast<int>(kBoardStatuses.size()); ++i) {
        const auto target = kBoardStatuses[static_cast<size_t>(i)];
        QAction* action = moveMenu->addAction(Theme::StatusStyle::label(target));
        action->setObjectName(u"StudyMoveTo%1"_s.arg(i));
        connect(action, &QAction::triggered, this, [this, i]() {
            const auto id = currentTopicId();
            const auto topic = id ? m_topics.byId(*id) : std::nullopt;
            if (!id || !topic) return;
            const int delta = i - boardStatusIndex(topic->status);
            if (delta != 0) emit moveStatusRequested(*id, delta);
        });
    }
    QAction* deleteAction = m_contextMenu->addAction(tr("Delete topic"));
    deleteAction->setObjectName(u"StudyDeleteTopicAction"_s);
    deleteAction->setToolTip(tr("Delete the topic and its checklist; logged study time is kept"));
    connect(deleteAction, &QAction::triggered, this, [this]() {
        if (const auto id = currentTopicId()) emit deleteRequested(*id);
    });

    refreshHeader();
}

int TopicBoardColumn::itemCount() const {
    return m_proxy ? m_proxy->rowCount() : 0;
}

int TopicBoardColumn::indexOfTopic(int id) const {
    if (!m_proxy) return -1;
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        if (m_proxy->index(row, 0).data(Models::TopicListModel::IdRole).toInt() == id) return row;
    }
    return -1;
}

std::optional<int> TopicBoardColumn::topicIdAt(int row) const {
    if (!m_proxy || row < 0 || row >= m_proxy->rowCount()) return std::nullopt;
    return m_proxy->index(row, 0).data(Models::TopicListModel::IdRole).toInt();
}

std::optional<int> TopicBoardColumn::currentTopicId() const {
    if (!m_list) return std::nullopt;
    const QModelIndex current = m_list->currentIndex();
    if (!current.isValid()) return std::nullopt;
    return current.data(Models::TopicListModel::IdRole).toInt();
}

bool TopicBoardColumn::selectTopicId(int id) {
    const int row = indexOfTopic(id);
    if (row < 0) return false;
    const QModelIndex idx = m_proxy->index(row, 0);
    m_list->setCurrentIndex(idx);
    m_list->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
    m_list->scrollTo(idx);
    if (m_restorer) {
        m_restorer->setSavedId(id);
    }
    return true;
}

void TopicBoardColumn::clearSelection() {
    if (m_list && m_list->selectionModel()) {
        const QSignalBlocker blocker(m_list->selectionModel());
        m_list->selectionModel()->clearSelection();
        m_list->selectionModel()->clearCurrentIndex();
    }
    if (m_restorer) {
        m_restorer->setSavedId(std::nullopt);
    }
}

void TopicBoardColumn::focusQuickAdd() {
    m_quickAdd->setFocus(Qt::OtherFocusReason);
    m_quickAdd->selectAll();
}

void TopicBoardColumn::setQuickAddTrack(int trackId, const QString& trackName) {
    m_quickAddTrackId = trackId;
    const QSignalBlocker blocker(m_quickAdd);
    if (trackName.isEmpty()) {
        m_quickAdd->setEnabled(false);
        m_quickAdd->setText({});
        m_quickAdd->setPlaceholderText(
            tr("Add a track with Manage tracks... to create topics"));
        m_quickAdd->setToolTip(tr("Topics need at least one track"));
    } else {
        m_quickAdd->setEnabled(true);
        m_quickAdd->setPlaceholderText(tr("New topic... (Adds to: %1)").arg(trackName));
        m_quickAdd->setToolTip(tr("Press Enter to add a topic to this column (Adds to: %1)").arg(trackName));
    }
}

bool TopicBoardColumn::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const bool alt = keyEvent->modifiers().testFlag(Qt::AltModifier);

        if (watched == m_quickAdd) {
            if (keyEvent->key() == Qt::Key_Escape) {
                if (!m_quickAdd->text().isEmpty()) {
                    m_quickAdd->clear();
                } else {
                    emit quickAddDismissed();
                }
                return true;
            }
            return QWidget::eventFilter(watched, event);
        }

        if (watched != m_list && watched != m_list->viewport()) {
            return QWidget::eventFilter(watched, event);
        }

        const auto id = currentTopicId();
        switch (keyEvent->key()) {
            case Qt::Key_Left:
                if (alt) {
                    if (id) emit moveStatusRequested(*id, -1);
                } else {
                    emit columnFocusRequested(-1);
                }
                return true;
            case Qt::Key_Right:
                if (alt) {
                    if (id) emit moveStatusRequested(*id, 1);
                } else {
                    emit columnFocusRequested(1);
                }
                return true;
            case Qt::Key_Up:
                if (alt) {
                    if (id) emit reorderRequested(*id, -1);
                    return true;
                }
                break;
            case Qt::Key_Down:
                if (alt) {
                    if (id) emit reorderRequested(*id, 1);
                    return true;
                }
                break;
            case Qt::Key_Delete:
                if (id) emit deleteRequested(*id);
                return true;
            default:
                break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TopicBoardColumn::showContextMenu(const QPoint& position) {
    const QModelIndex index = m_list->indexAt(position);
    if (!index.isValid()) return;
    m_list->setCurrentIndex(index);
    const auto id = currentTopicId();
    if (!id) return;

    const auto topic = m_topics.byId(*id);
    const int currentIndex = topic ? boardStatusIndex(topic->status) : 0;

    auto* moveMenu = m_contextMenu->findChild<QMenu*>(u"StudyMoveToMenu"_s);
    if (moveMenu) {
        const auto actions = moveMenu->actions();
        for (int i = 0; i < actions.size() && i < static_cast<int>(kBoardStatuses.size()); ++i) {
            actions.at(i)->setEnabled(i != currentIndex);
        }
    }
    m_contextMenu->exec(m_list->viewport()->mapToGlobal(position));
}

void TopicBoardColumn::submitQuickAdd() {
    const QString title = m_quickAdd->text().trimmed();
    if (title.isEmpty() || !m_quickAdd->isEnabled()) return;
    const QSignalBlocker blocker(m_quickAdd);
    m_quickAdd->clear();
    emit quickAddSubmitted(title, m_quickAddTrackId, m_status);
}

void TopicBoardColumn::refreshHeader() {
    m_countLabel->setText(QString::number(itemCount()));
}



void TopicBoardColumn::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (m_list && m_list->isDragOver()) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const auto& tokens = m_theme.currentTokens();
        QPen pen(tokens.accent);
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        QColor bg = tokens.accent;
        bg.setAlpha(15);
        p.setBrush(bg);
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
    }
}

}  // namespace JobPrep::Ui::Widgets
