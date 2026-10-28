#include "ui/pages/StudyPage.h"

#include <algorithm>
#include <QButtonGroup>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"
#include "models/TopicTableProxy.h"
#include "services/SettingsService.h"
#include "ui/StudyLogging.h"
#include "ui/delegates/ProgressBarDelegate.h"
#include "ui/delegates/StatusBadgeDelegate.h"
#include "ui/dialogs/SessionDialog.h"
#include "ui/dialogs/TrackDialog.h"
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/SelectionRestorer.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "ui/widgets/TopicListView.h"

using namespace Qt::StringLiterals;
using JobPrep::Models::TopicTableProxy;

namespace JobPrep::Ui::Pages {

namespace {

constexpr std::array<const char*, 4> kColumnNames = {"Backlog", "InProgress", "Review", "Done"};

}  // namespace

StudyPage::StudyPage(Data::TopicRepository& topics,
                     Data::TrackRepository& tracks,
                     Data::SessionRepository& sessions,
                     Services::SettingsService& settings,
                     Theme::ThemeManager& theme,
                     QWidget* parent,
                     std::function<QDate()> todayProvider)
    : PageBase(tr("Study"), parent),
      m_topics(topics),
      m_tracks(tracks),
      m_sessions(sessions),
      m_settings(settings),
      m_theme(theme),
      m_todayProvider(todayProvider ? std::move(todayProvider)
                                    : std::function<QDate()>([] { return QDate::currentDate(); })),
      m_trackFilter(settings.studyTrackFilter()) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 16, 24, 16);
    rootLayout->setSpacing(16);

    m_model = new Models::TopicListModel(topics, tracks, this, m_todayProvider);
    connect(m_model, &QAbstractItemModel::modelAboutToBeReset, this, [this] { m_inReset = true; });
    connect(m_model, &QAbstractItemModel::modelReset, this, &StudyPage::onModelReset);
    connect(&m_topics, &Data::TopicRepository::changed, this, &StudyPage::onTopicsChanged);
    connect(&m_tracks, &Data::TrackRepository::changed, this, &StudyPage::onTracksChanged);

    setupUi(rootLayout);

    refreshChips();
    applyFilters();
    refreshViewState();

    const QByteArray splitterState = m_settings.studySplitterState();
    if (splitterState.isEmpty()) {
        m_splitter->setSizes({780, 320});
    } else {
        m_splitter->restoreState(splitterState);
    }
    setDetailVisible(false);

    // Initial view mode
    const QString mode = m_settings.studyViewMode();
    setViewMode(mode == u"list"_s ? u"list"_s : u"board"_s);
}

void StudyPage::setupUi(QVBoxLayout* layout) {
    setupToolbar(layout);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(u"StudyMessageLabel"_s);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->hide();
    layout->addWidget(m_messageLabel);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(u"StudyStack"_s);

    // Index 0: No topics (tracks exist, but topics empty)
    auto* noDataState = new Widgets::EmptyState(
        u"study"_s, tr("No Study Topics"),
        tr("Organize your C++ and English interview topics into kanban columns and log study sessions."),
        u"+ Topic"_s, m_stack);
    noDataState->setObjectName(u"StudyEmptyState"_s);
    connect(noDataState, &Widgets::EmptyState::actionClicked, this, &StudyPage::newTopic);
    m_stack->addWidget(noDataState);

    // Index 1: Views splitter (Board / List + Detail Panel)
    setupSplitterAndViews();
    m_stack->addWidget(m_splitter);

    // Index 2: No matches
    auto* noMatchesState = new Widgets::EmptyState(
        u"search"_s, tr("No Matches"),
        tr("No topic matches the current search and track filter."),
        tr("Clear filters"), m_stack);
    noMatchesState->setObjectName(u"StudyNoMatchesState"_s);
    connect(noMatchesState, &Widgets::EmptyState::actionClicked, this, [this]() {
        setSearchText(QString());
        onChipSelected(kAllTracks);
    });
    m_stack->addWidget(noMatchesState);

    // Index 3: No tracks
    auto* noTracksState = new Widgets::EmptyState(
        u"study"_s, tr("No Study Tracks"),
        tr("Create a study track (like C++ or English) to start planning topics."),
        tr("+ Track"), m_stack);
    noTracksState->setObjectName(u"StudyNoTracksState"_s);
    connect(noTracksState, &Widgets::EmptyState::actionClicked, this, &StudyPage::openTrackDialog);
    m_stack->addWidget(noTracksState);

    layout->addWidget(m_stack, 1);
}

void StudyPage::setupToolbar(QVBoxLayout* layout) {
    auto* bar = new QWidget(this);
    bar->setObjectName(u"StudyToolbar"_s);
    auto* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(8);

    m_chips = new Widgets::ChipBar(bar);
    m_chips->setObjectName(u"StudyTrackChips"_s);
    barLayout->addWidget(m_chips, 0);

    m_addTrackBtn = new QPushButton(tr("+ Track"), bar);
    m_addTrackBtn->setObjectName(u"StudyAddTrackBtn"_s);
    m_addTrackBtn->setToolTip(tr("Manage tracks..."));
    m_addTrackBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(m_addTrackBtn, 0);

    m_manageTracksBtn = new QPushButton(tr("Manage tracks..."), bar);
    m_manageTracksBtn->setObjectName(u"StudyManageTracksBtn"_s);
    m_manageTracksBtn->setToolTip(tr("Add, rename, recolor or delete tracks"));
    m_manageTracksBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(m_manageTracksBtn, 0);

    // View mode toggle [Board | List]
    auto* viewToggle = new QWidget(bar);
    auto* toggleLayout = new QHBoxLayout(viewToggle);
    toggleLayout->setContentsMargins(0, 0, 0, 0);
    toggleLayout->setSpacing(2);

    m_boardBtn = new QPushButton(tr("Board"), viewToggle);
    m_boardBtn->setObjectName(u"StudyViewBoardBtn"_s);
    m_boardBtn->setCheckable(true);
    m_boardBtn->setChecked(true);
    m_boardBtn->setCursor(Qt::PointingHandCursor);

    m_listBtn = new QPushButton(tr("List"), viewToggle);
    m_listBtn->setObjectName(u"StudyViewListBtn"_s);
    m_listBtn->setCheckable(true);
    m_listBtn->setCursor(Qt::PointingHandCursor);

    toggleLayout->addWidget(m_boardBtn);
    toggleLayout->addWidget(m_listBtn);
    barLayout->addWidget(viewToggle, 0);

    barLayout->addStretch(1);

    m_searchEdit = new QLineEdit(bar);
    m_searchEdit->setObjectName(u"StudySearchEdit"_s);
    m_searchEdit->setPlaceholderText(tr("Search topics (Ctrl+F)"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(320);
    m_searchEdit->addAction(Theme::IconProvider::icon(u"search"_s, m_theme.currentTokens().textMuted),
                            QLineEdit::LeadingPosition);
    barLayout->addWidget(m_searchEdit, 0, Qt::AlignVCenter);

    auto* addTopicBtn = new QPushButton(tr("+ Topic"), bar);
    addTopicBtn->setObjectName(u"StudyAddTopicBtn"_s);
    addTopicBtn->setProperty("primary", true);
    addTopicBtn->setToolTip(tr("Add a topic to Backlog (Ctrl+N)"));
    addTopicBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(addTopicBtn, 0);

    layout->addWidget(bar);

    connect(m_chips, &Widgets::ChipBar::chipSelected, this, &StudyPage::onChipSelected);
    connect(m_addTrackBtn, &QPushButton::clicked, this, &StudyPage::openTrackDialog);
    connect(m_manageTracksBtn, &QPushButton::clicked, this, &StudyPage::openTrackDialog);
    connect(m_boardBtn, &QPushButton::clicked, this, [this] { setViewMode(u"board"_s); });
    connect(m_listBtn, &QPushButton::clicked, this, [this] { setViewMode(u"list"_s); });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StudyPage::setSearchText);
    connect(addTopicBtn, &QPushButton::clicked, this, &StudyPage::newTopic);

    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this]() {
        const auto tokens = m_theme.currentTokens();
        const auto actions = m_searchEdit->actions();
        if (!actions.isEmpty()) {
            actions.first()->setIcon(Theme::IconProvider::icon(u"search"_s, tokens.textMuted));
        }
    });
}

void StudyPage::setupSplitterAndViews() {
    m_splitter = new QSplitter(Qt::Horizontal, m_stack);
    m_splitter->setObjectName(u"StudySplitter"_s);
    m_splitter->setChildrenCollapsible(false);

    m_viewStack = new QStackedWidget(m_splitter);
    m_viewStack->setObjectName(u"StudyViewStack"_s);

    setupBoardView();
    m_viewStack->addWidget(m_boardScroll);

    setupTableView();
    m_viewStack->addWidget(m_tableView);

    m_splitter->addWidget(m_viewStack);

    setupDetailPanel();
    m_splitter->addWidget(m_detail);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);

    connect(m_splitter, &QSplitter::splitterMoved, this, [this] {
        m_settings.setStudySplitterState(m_splitter->saveState());
    });
}

void StudyPage::setupBoardView() {
    m_boardScroll = new QScrollArea(m_viewStack);
    m_boardScroll->setObjectName(u"StudyBoardScroll"_s);
    m_boardScroll->setWidgetResizable(true);
    m_boardScroll->setFrameShape(QFrame::NoFrame);
    m_boardScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_boardScroll->setMinimumWidth(420);

    auto* board = new QWidget(m_boardScroll);
    board->setObjectName(u"StudyBoard"_s);
    auto* boardLayout = new QHBoxLayout(board);
    boardLayout->setContentsMargins(0, 0, 4, 0);
    boardLayout->setSpacing(12);

    for (int i = 0; i < static_cast<int>(Widgets::kBoardStatuses.size()); ++i) {
        const auto status = Widgets::kBoardStatuses[static_cast<size_t>(i)];
        auto* column = new Widgets::TopicBoardColumn(status, m_model, m_topics, m_theme, board);
        column->setObjectName(u"StudyColumn%1"_s.arg(QLatin1StringView(kColumnNames[static_cast<size_t>(i)])));
        column->setMinimumWidth(220);
        boardLayout->addWidget(column, 1);
        m_columns.append(column);

        connect(column, &Widgets::TopicBoardColumn::topicActivated, this,
                &StudyPage::onColumnActivated);
        connect(column, &Widgets::TopicBoardColumn::deleteRequested, this,
                &StudyPage::onDeleteRequested);
        connect(column, &Widgets::TopicBoardColumn::moveStatusRequested, this,
                &StudyPage::onMoveStatusRequested);
        connect(column, &Widgets::TopicBoardColumn::reorderRequested, this,
                &StudyPage::onReorderRequested);
        connect(column, &Widgets::TopicBoardColumn::quickAddSubmitted, this,
                &StudyPage::onQuickAddSubmitted);
        connect(column, &Widgets::TopicBoardColumn::quickAddDismissed, this,
                [this] { refreshViewState(); });
        connect(column, &Widgets::TopicBoardColumn::columnFocusRequested, this,
                [this, i](int delta) { focusColumn(i, delta); });
        connect(column->proxy(), &Models::TopicBoardProxy::dropFailed, this,
                [this](const QString& reason) {
                    onWriteError(tr("Could not move the topic."), reason);
                });
        connect(column->list()->selectionModel(), &QItemSelectionModel::currentChanged, this,
                [this, column](const QModelIndex& current, const QModelIndex&) {
                    onColumnCurrentChanged(column, current);
                });
        connect(column->list()->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                [this, column](const QItemSelection&, const QItemSelection&) {
                    onColumnSelectionChanged(column);
                });
    }

    boardLayout->addStretch(1);
    m_boardScroll->setWidget(board);
}

void StudyPage::setupTableView() {
    auto* tableProxy = new TopicTableProxy(this);
    tableProxy->setSourceModel(m_model);
    tableProxy->setSortRole(Models::TopicListModel::SortRole);
    m_tableProxy = tableProxy;

    m_tableView = new QTableView(m_viewStack);
    m_tableView->setObjectName(u"StudyTableView"_s);
    m_tableView->setModel(m_tableProxy);
    m_tableView->setSortingEnabled(true);
    m_tableView->sortByColumn(Models::TopicListModel::TrackColumn, Qt::AscendingOrder);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setDragEnabled(false);
    m_tableView->setAcceptDrops(false);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->installEventFilter(this);

    m_tableView->setItemDelegateForColumn(
        Models::TopicListModel::StatusColumn,
        new Delegates::StatusBadgeDelegate(Delegates::StatusBadgeDelegate::Mode::Topic, m_tableView));
    m_tableView->setItemDelegateForColumn(
        Models::TopicListModel::ProgressColumn,
        new Delegates::ProgressBarDelegate(m_theme, m_tableView));

    m_tableRestorer = new Widgets::SelectionRestorer(m_tableView, Models::TopicListModel::IdRole, this);

    connect(m_tableView, &QTableView::activated, this, &StudyPage::onTableRowActivated);
    connect(m_tableView, &QTableView::clicked, this, &StudyPage::onTableRowActivated);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            &StudyPage::onTableSelectionChanged);
}

void StudyPage::setupDetailPanel() {
    m_detail = new Widgets::TopicDetailPanel(m_topics, m_theme, m_splitter);
    m_detail->setObjectName(u"StudyDetailPanel"_s);
    m_detail->setTracks(m_tracks.all());
    m_detail->setMinimumWidth(280);

    connect(m_detail, &Widgets::TopicDetailPanel::writeFailed, this,
            [this](const QString& error) { onWriteError(tr("Could not save the topic."), error); });
    connect(m_detail, &Widgets::TopicDetailPanel::closeRequested, this,
            [this] { setDetailVisible(false); });
    connect(m_detail, &Widgets::TopicDetailPanel::logTimeRequested, this,
            [this](int topicId) { openSessionDialog(topicId); });
}

void StudyPage::focusSearch() {
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void StudyPage::triggerNew() {
    newTopic();
}

void StudyPage::flushEdits() {
    m_detail->flush();
}

void StudyPage::hideEvent(QHideEvent* event) {
    m_detail->flush();
    PageBase::hideEvent(event);
}

bool StudyPage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_tableView && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            if (m_selectedTopicId.has_value()) {
                onDeleteRequested(*m_selectedTopicId);
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (m_selectedTopicId.has_value()) {
                setDetailVisible(true);
                m_detail->focusFirstEditor();
                return true;
            }
        }
    }
    return PageBase::eventFilter(watched, event);
}

void StudyPage::newTopic() {
    if (m_tracks.isEmpty()) {
        openTrackDialog();
        return;
    }
    if (m_settings.studyViewMode() == u"list"_s) {
        setViewMode(u"board"_s);
    }
    if (m_stack->currentIndex() != 1) m_stack->setCurrentIndex(1);
    if (!m_columns.isEmpty()) {
        m_columns.first()->focusQuickAdd();
        m_boardScroll->ensureWidgetVisible(m_columns.first());
    }
}

void StudyPage::openTrackDialog() {
    Dialogs::TrackDialog dialog(m_tracks, m_topics, this);
    dialog.exec();
}

void StudyPage::openSessionDialog(std::optional<int> topicId) {
    Dialogs::SessionDialog dialog(m_sessions, m_topics, this, m_todayProvider);
    if (topicId.has_value() && *topicId > 0) {
        dialog.setTopicId(*topicId);
    } else if (m_selectedTopicId.has_value() && *m_selectedTopicId > 0) {
        dialog.setTopicId(*m_selectedTopicId);
    }
    dialog.exec();
}

void StudyPage::setViewMode(const QString& mode) {
    const QString target = (mode == u"list"_s) ? u"list"_s : u"board"_s;
    m_settings.setStudyViewMode(target);

    const bool isList = (target == u"list"_s);
    m_boardBtn->setChecked(!isList);
    m_listBtn->setChecked(isList);
    m_viewStack->setCurrentIndex(isList ? 1 : 0);

    if (m_selectedTopicId.has_value()) {
        selectTopic(*m_selectedTopicId);
    }
}

QString StudyPage::viewMode() const {
    return m_settings.studyViewMode();
}

bool StudyPage::selectTopic(int id) {
    const auto topic = m_topics.byId(id);
    if (!topic) return false;

    if (m_trackFilter != kAllTracks && m_trackFilter != topic->trackId) {
        onChipSelected(kAllTracks);
    }
    if (!m_searchText.isEmpty() && !matchesSearch(topic->title, topic->tags)) {
        setSearchText(QString());
    }

    m_selectedTopicId = id;

    // Restore in board: select in target column and clear all other columns
    auto* targetCol = columnContaining(id);
    for (auto* col : m_columns) {
        if (col == targetCol) {
            col->selectTopicId(id);
            m_boardScroll->ensureWidgetVisible(col);
        } else {
            col->clearSelection();
        }
    }

    // Restore in table
    for (int row = 0; row < m_tableProxy->rowCount(); ++row) {
        const QModelIndex idx = m_tableProxy->index(row, 0);
        if (idx.data(Models::TopicListModel::IdRole).toInt() == id) {
            m_tableView->setCurrentIndex(idx);
            m_tableView->scrollTo(idx);
            break;
        }
    }

    m_detail->setTracks(m_tracks.all());
    m_detail->loadFrom(id);
    setDetailVisible(true);
    return true;
}

void StudyPage::setDetailVisible(bool visible) {
    if (m_detail->isVisible() == visible) return;
    if (!visible) m_detail->flush();
    m_detail->setVisible(visible);
}

bool StudyPage::isDetailVisible() const {
    return m_detail->isVisible();
}

void StudyPage::setSearchText(const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_searchText == trimmed) return;
    m_searchText = trimmed;
    if (m_searchEdit->text() != trimmed) {
        const QSignalBlocker blocker(m_searchEdit);
        m_searchEdit->setText(trimmed);
    }
    applyFilters();
}

QString StudyPage::searchText() const {
    return m_searchText;
}

int StudyPage::currentViewIndex() const {
    return m_stack->currentIndex();
}

int StudyPage::visibleTopicCount() const {
    if (m_settings.studyViewMode() == u"list"_s && m_tableProxy) {
        return m_tableProxy->rowCount();
    }
    int count = 0;
    for (const auto* column : m_columns) count += column->itemCount();
    return count;
}

void StudyPage::onTopicsChanged() {
    qCDebug(lcStudy) << "TopicRepository::changed";
    m_detail->refreshFromRepository();
    refreshViewState();
}

void StudyPage::onTracksChanged() {
    refreshChips();
    m_detail->setTracks(m_tracks.all());
    applyFilters();
    refreshViewState();
}

void StudyPage::onModelReset() {
    m_inReset = true;
    if (!m_selectedTopicId.has_value()) {
        for (auto* col : m_columns) col->clearSelection();
        m_detail->clearTopic();
        m_inReset = false;
        return;
    }
    const int topicId = *m_selectedTopicId;
    const auto topic = m_topics.byId(topicId);
    if (!topic) {
        m_selectedTopicId.reset();
        for (auto* col : m_columns) col->clearSelection();
        m_detail->clearTopic();
        m_inReset = false;
        return;
    }

    auto* targetCol = columnContaining(topicId);
    for (auto* col : m_columns) {
        if (col == targetCol) {
            col->selectTopicId(topicId);
        } else {
            col->clearSelection();
        }
    }
    if (m_settings.studyViewMode() == u"list"_s && m_tableView && m_tableProxy) {
        for (int row = 0; row < m_tableProxy->rowCount(); ++row) {
            const auto idx = m_tableProxy->index(row, 0);
            if (idx.data(Models::TopicListModel::IdRole).toInt() == topicId) {
                m_tableView->setCurrentIndex(idx);
                m_tableView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                break;
            }
        }
    }
    m_selectedTopicId = topicId;
    m_detail->refreshFromRepository();
    m_inReset = false;
}

void StudyPage::onChipSelected(int id) {
    if (m_trackFilter == id) return;
    m_trackFilter = id;
    m_settings.setStudyTrackFilter(id);
    m_chips->setCurrent(id);
    applyFilters();
}

void StudyPage::onColumnActivated(int id) {
    m_selectedTopicId = id;
    m_detail->setTracks(m_tracks.all());
    m_detail->loadFrom(id);
    setDetailVisible(true);
    m_detail->focusFirstEditor();
}

void StudyPage::onTableRowActivated(const QModelIndex& index) {
    if (!index.isValid()) return;
    const int id = index.data(Models::TopicListModel::IdRole).toInt();
    if (id <= 0) return;
    m_selectedTopicId = id;
    m_detail->setTracks(m_tracks.all());
    m_detail->loadFrom(id);
    setDetailVisible(true);
    m_detail->focusFirstEditor();
}

void StudyPage::onTableSelectionChanged() {
    if (m_inReset) return;
    const QModelIndex current = m_tableView->currentIndex();
    if (!current.isValid()) {
        if (!m_selectedTopicId) return;
        m_detail->flush();
        m_detail->clearTopic();
        m_selectedTopicId.reset();
        return;
    }
    const int id = current.data(Models::TopicListModel::IdRole).toInt();
    if (m_selectedTopicId == id) return;
    m_selectedTopicId = id;
    m_detail->setTracks(m_tracks.all());
    m_detail->loadFrom(id);
    if (!m_suppressDetailOpen) setDetailVisible(true);
}

void StudyPage::onDeleteRequested(int id) {
    const auto topic = m_topics.byId(id);
    if (!topic) return;
    const auto answer = QMessageBox::question(
        this, tr("Delete Topic"),
        tr("Delete \u201c%1\u201d? Its checklist goes away; logged study time is kept.")
            .arg(topic->title),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    if (!m_topics.remove(id)) {
        onWriteError(tr("Could not delete the topic."), m_topics.lastError());
    }
}

void StudyPage::onMoveStatusRequested(int id, int delta) {
    const auto topic = m_topics.byId(id);
    if (!topic) return;
    const int from = Widgets::boardStatusIndex(topic->status);
    const int to = std::clamp(from + delta, 0, static_cast<int>(Widgets::kBoardStatuses.size()) - 1);
    if (to == from) return;
    moveTopicTo(id, Widgets::kBoardStatuses[static_cast<size_t>(to)]);
}

void StudyPage::onReorderRequested(int id, int delta) {
    auto* column = columnContaining(id);
    if (!column) return;
    QList<int> orderedIds;
    for (int row = 0; row < column->itemCount(); ++row) {
        orderedIds.append(*column->topicIdAt(row));
    }
    const int from = static_cast<int>(orderedIds.indexOf(id));
    if (from < 0) return;
    const int to = std::clamp(from + delta, 0, static_cast<int>(orderedIds.size()) - 1);
    if (to == from) return;
    orderedIds.move(from, to);
    if (!m_topics.moveTopic(id, column->status(), orderedIds)) {
        onWriteError(tr("Could not reorder the topic."), m_topics.lastError());
    }
}

void StudyPage::onQuickAddSubmitted(const QString& title, int trackId,
                                    Domain::TopicStatus status) {
    Domain::Topic topic;
    topic.trackId = trackId;
    topic.title = title;
    topic.status = status;
    topic.priority = Domain::Priority::Normal;
    const auto all = m_topics.all();
    topic.position = all.isEmpty() ? 0 : all.last().position + 1;
    if (!m_topics.insert(topic)) {
        onWriteError(tr("Could not add the topic."), m_topics.lastError());
        return;
    }
    if (auto* column = columnFor(status)) {
        m_suppressDetailOpen = true;
        column->selectTopicId(topic.id);
        m_suppressDetailOpen = false;
    }
}

void StudyPage::onWriteError(const QString& what, const QString& error) {
    qWarning() << what << error;
    m_model->reload();
    showMessage(error.isEmpty() ? what : u"%1 %2"_s.arg(what, error), "error");
}

void StudyPage::onColumnSelectionChanged(Widgets::TopicBoardColumn* activeColumn) {
    if (m_inReset) return;
    if (!activeColumn || !activeColumn->list() || !activeColumn->list()->selectionModel()) return;

    const auto selected = activeColumn->list()->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        bool anySelected = false;
        for (const auto* col : m_columns) {
            if (col->list()->selectionModel() && col->list()->selectionModel()->hasSelection()) {
                anySelected = true;
                break;
            }
        }
        if (!anySelected) {
            if (m_selectedTopicId) {
                qCDebug(lcStudy) << "selectionChanged: topic=none";
                m_detail->flush();
                m_detail->clearTopic();
                m_selectedTopicId.reset();
            }
        }
        return;
    }

    const QModelIndex current = selected.first();
    const int id = current.data(Models::TopicListModel::IdRole).toInt();

    if (activeColumn->list()->currentIndex() != current) {
        const QSignalBlocker blocker(activeColumn->list()->selectionModel());
        activeColumn->list()->setCurrentIndex(current);
    }

    for (auto* col : m_columns) {
        if (col != activeColumn) {
            col->clearSelection();
        }
    }

    qCDebug(lcStudy) << "selectionChanged: topic=" << id;

    if (m_selectedTopicId == id) return;
    m_selectedTopicId = id;
    m_detail->setTracks(m_tracks.all());
    m_detail->loadFrom(id);
    if (!m_suppressDetailOpen) setDetailVisible(true);
}

void StudyPage::onColumnCurrentChanged(Widgets::TopicBoardColumn* activeColumn, const QModelIndex& current) {
    if (m_inReset) return;
    if (!current.isValid()) return;
    if (activeColumn->list()->selectionModel() && !activeColumn->list()->selectionModel()->isSelected(current)) {
        activeColumn->list()->selectionModel()->select(current, QItemSelectionModel::ClearAndSelect);
        return;
    }
    onColumnSelectionChanged(activeColumn);
}

void StudyPage::focusColumn(int fromColumn, int delta) {
    const int target = fromColumn + delta;
    if (target < 0 || target >= m_columns.size()) return;
    const int row = [&]() {
        if (auto* from = m_columns.value(fromColumn, nullptr)) {
            const QModelIndex current = from->list()->currentIndex();
            if (current.isValid()) return current.row();
        }
        return 0;
    }();
    auto* column = m_columns.at(target);
    column->list()->setFocus(Qt::OtherFocusReason);
    if (column->itemCount() == 0) {
        for (auto* col : m_columns) col->clearSelection();
        m_selectedTopicId.reset();
        m_detail->clearTopic();
        return;
    }
    const int clamped = std::clamp(row, 0, column->itemCount() - 1);
    const QModelIndex targetIndex = column->proxy()->index(clamped, 0);
    const int id = targetIndex.data(Models::TopicListModel::IdRole).toInt();

    m_selectedTopicId = id;
    for (auto* col : m_columns) {
        if (col == column) {
            col->selectTopicId(id);
        } else {
            col->clearSelection();
        }
    }
    m_detail->setTracks(m_tracks.all());
    m_detail->loadFrom(id);
    if (!m_suppressDetailOpen) setDetailVisible(true);
}

void StudyPage::moveTopicTo(int id, Domain::TopicStatus status) {
    auto* target = columnFor(status);
    if (!target) return;
    QList<int> orderedIds;
    orderedIds.reserve(target->itemCount() + 1);
    for (int row = 0; row < target->itemCount(); ++row) {
        orderedIds.append(*target->topicIdAt(row));
    }
    if (!orderedIds.contains(id)) orderedIds.append(id);
    m_selectedTopicId = id;
    if (!m_topics.moveTopic(id, status, orderedIds)) {
        onWriteError(tr("Could not move the topic."), m_topics.lastError());
    }
}

void StudyPage::showMessage(const QString& text, const char* kind) {
    m_messageLabel->setText(text);
    m_messageLabel->setProperty("kind", QByteArray(kind));
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    m_messageLabel->show();
    QTimer::singleShot(6000, m_messageLabel, &QWidget::hide);
}

bool StudyPage::matchesSearch(const QString& title, const QString& tags) const {
    if (m_searchText.isEmpty()) return true;
    return title.contains(m_searchText, Qt::CaseInsensitive) ||
           tags.contains(m_searchText, Qt::CaseInsensitive);
}

Widgets::TopicBoardColumn* StudyPage::columnFor(Domain::TopicStatus status) const {
    for (auto* column : m_columns) {
        if (column->status() == status) return column;
    }
    return nullptr;
}

Widgets::TopicBoardColumn* StudyPage::columnContaining(int id) const {
    for (auto* column : m_columns) {
        if (column->indexOfTopic(id) >= 0) return column;
    }
    return nullptr;
}

std::optional<int> StudyPage::selectedTopicId() const {
    if (m_settings.studyViewMode() == u"list"_s) {
        if (m_tableView) {
            const QModelIndex current = m_tableView->currentIndex();
            if (current.isValid()) return current.data(Models::TopicListModel::IdRole).toInt();
        }
        return std::nullopt;
    }
    return m_selectedTopicId;
}

void StudyPage::refreshChips() {
    m_chips->clearChips();
    m_chips->addChip(tr("All"), kAllTracks);
    for (const auto& track : m_tracks.all()) {
        m_chips->addChip(track.name, track.id);
    }
    if (m_trackFilter != kAllTracks && !m_tracks.byId(m_trackFilter)) {
        m_trackFilter = kAllTracks;
        m_settings.setStudyTrackFilter(kAllTracks);
    }
    m_chips->setCurrent(m_trackFilter);
}

void StudyPage::applyFilters() {
    const std::optional<int> track =
        m_trackFilter == kAllTracks ? std::nullopt : std::optional<int>(m_trackFilter);
    for (auto* column : m_columns) {
        column->proxy()->setTrackFilter(track);
        column->proxy()->setSearchFilter(m_searchText);
    }

    if (m_tableProxy) {
        auto* tableProxy = static_cast<TopicTableProxy*>(m_tableProxy);
        tableProxy->setTrackFilter(track);
        tableProxy->setSearchFilter(m_searchText);
    }

    if (m_selectedTopicId) {
        if (columnContaining(*m_selectedTopicId)) {
            auto* column = columnContaining(*m_selectedTopicId);
            if (!column->list()->currentIndex().isValid()) {
                column->selectTopicId(*m_selectedTopicId);
            }
        } else {
            m_detail->flush();
            m_detail->clearTopic();
            m_selectedTopicId.reset();
        }
    }

    updateQuickAddTracks();
    refreshViewState();
}

void StudyPage::refreshViewState() {
    if (m_tracks.isEmpty()) {
        m_stack->setCurrentIndex(3);
    } else if (m_model->rowCount() == 0) {
        m_stack->setCurrentIndex(0);
    } else if (visibleTopicCount() == 0) {
        m_stack->setCurrentIndex(2);
    } else {
        m_stack->setCurrentIndex(1);
    }
}

void StudyPage::updateQuickAddTracks() {
    int trackId = 0;
    QString trackName;
    if (m_trackFilter != kAllTracks) {
        if (const auto track = m_tracks.byId(m_trackFilter)) {
            trackId = track->id;
            trackName = track->name;
        }
    } else {
        const auto tracks = m_tracks.all();
        if (!tracks.isEmpty()) {
            trackId = tracks.first().id;
            trackName = tracks.first().name;
        }
    }
    for (auto* column : m_columns) {
        column->setQuickAddTrack(trackId, trackName);
    }
}

}  // namespace JobPrep::Ui::Pages
