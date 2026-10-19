#include "ui/pages/StudyPage.h"

#include <algorithm>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include "data/DataLogging.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "models/TopicBoardProxy.h"
#include "models/TopicListModel.h"
#include "services/SettingsService.h"
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

namespace {

const char* kColumnNames[] = {"Backlog", "InProgress", "Review", "Done"};

}  // namespace

StudyPage::StudyPage(Data::TopicRepository& topics, Data::TrackRepository& tracks,
                     Services::SettingsService& settings, Theme::ThemeManager& theme,
                     QWidget* parent, std::function<QDate()> todayProvider)
    : PageBase(tr("Study Tracks"), parent),
      m_topics(topics),
      m_tracks(tracks),
      m_settings(settings),
      m_theme(theme),
      m_todayProvider(todayProvider ? std::move(todayProvider)
                                    : std::function<QDate()>([] { return QDate::currentDate(); })) {
    m_model = new Models::TopicListModel(topics, tracks, this, m_todayProvider);
    // Connected before the columns exist, so the flag is set before any per-column
    // SelectionRestorer reacts to the same reset.
    connect(m_model, &QAbstractItemModel::modelAboutToBeReset, this,
            [this] { m_inReset = true; });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(12);
    setupUi(layout);

    // Connected after the columns, so it runs after every SelectionRestorer restored.
    connect(m_model, &QAbstractItemModel::modelReset, this, &StudyPage::onModelReset);
    connect(&topics, &Data::TopicRepository::changed, this, &StudyPage::onTopicsChanged);
    connect(&tracks, &Data::TrackRepository::changed, this, &StudyPage::onTracksChanged);

    // Ensure detail panel tracks are up to date (constructor may have set with empty list).
    m_detail->setTracks(m_tracks.all());
    m_trackFilter = m_settings.studyTrackFilter();
    refreshChips();
    applyFilters();
    refreshViewState();

    const QByteArray splitterState = m_settings.studySplitterState();
    if (splitterState.isEmpty()) {
        m_splitter->setSizes({760, 320});
    } else {
        m_splitter->restoreState(splitterState);
    }
    setDetailVisible(false);
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

    auto* noDataState = new Widgets::EmptyState(
        u"study"_s, tr("No Study Topics"),
        tr("Organize your C++ and English interview topics into kanban columns and log study sessions."),
        u"+ Topic"_s, m_stack);
    noDataState->setObjectName(u"StudyEmptyState"_s);
    connect(noDataState, &Widgets::EmptyState::actionClicked, this, &StudyPage::newTopic);
    m_stack->addWidget(noDataState);

    setupBoard();

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

    auto* manageTracksBtn = new QPushButton(tr("Manage tracks..."), bar);
    manageTracksBtn->setObjectName(u"StudyManageTracksBtn"_s);
    manageTracksBtn->setEnabled(false);
    manageTracksBtn->setToolTip(tr("Arrives in M4 step C"));
    barLayout->addWidget(manageTracksBtn, 0);

    barLayout->addStretch(1);

    m_searchEdit = new QLineEdit(bar);
    m_searchEdit->setObjectName(u"StudySearchEdit"_s);
    m_searchEdit->setPlaceholderText(tr("Search topics (Ctrl+F)"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(320);
    m_searchEdit->addAction(Theme::IconProvider::icon(u"search"_s, m_theme.currentTokens().textMuted),
                            QLineEdit::LeadingPosition);
    barLayout->addWidget(m_searchEdit, 0);

    auto* addTopicBtn = new QPushButton(tr("+ Topic"), bar);
    addTopicBtn->setObjectName(u"StudyAddTopicBtn"_s);
    addTopicBtn->setProperty("primary", true);
    addTopicBtn->setToolTip(tr("Add a topic to Backlog (Ctrl+N)"));
    addTopicBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(addTopicBtn, 0);

    layout->addWidget(bar);

    connect(m_chips, &Widgets::ChipBar::chipSelected, this, &StudyPage::onChipSelected);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StudyPage::setSearchText);
    connect(addTopicBtn, &QPushButton::clicked, this, &StudyPage::newTopic);
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this]() {
        const auto tokens = m_theme.currentTokens();
        const auto actions = m_searchEdit->actions();
        if (!actions.isEmpty()) {
            actions.first()->setIcon(
                Theme::IconProvider::icon(u"search"_s, tokens.textMuted));
        }
    });
}

void StudyPage::setupBoard() {
    m_splitter = new QSplitter(Qt::Horizontal, m_stack);
    m_splitter->setObjectName(u"StudySplitter"_s);
    m_splitter->setChildrenCollapsible(false);

    m_boardScroll = new QScrollArea(m_splitter);
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
        column->setObjectName(u"StudyColumn%1"_s.arg(QLatin1StringView(kColumnNames[i])));
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
                [this](const QModelIndex& current, const QModelIndex&) {
                    onCurrentChanged(current);
                });
    }

    boardLayout->addStretch(1);
    m_boardScroll->setWidget(board);
    m_splitter->addWidget(m_boardScroll);

    setupDetailPanel();
    m_splitter->addWidget(m_detail);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);

    connect(m_splitter, &QSplitter::splitterMoved, this, [this] {
        m_settings.setStudySplitterState(m_splitter->saveState());
    });

    m_stack->addWidget(m_splitter);
}

void StudyPage::setupDetailPanel() {
    m_detail = new Widgets::TopicDetailPanel(m_topics, m_theme, m_splitter);
    m_detail->setTracks(m_tracks.all());
    m_detail->setMinimumWidth(280);
    connect(m_detail, &Widgets::TopicDetailPanel::writeFailed, this,
            [this](const QString& error) { onWriteError(tr("Could not save the topic."), error); });
    connect(m_detail, &Widgets::TopicDetailPanel::closeRequested, this,
            [this] { setDetailVisible(false); });
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

void StudyPage::newTopic() {
    if (m_tracks.isEmpty()) {
        showMessage(tr("Add a track with Manage tracks... to create topics."), "hint");
        return;
    }
    if (m_stack->currentIndex() != 1) m_stack->setCurrentIndex(1);
    if (!m_columns.isEmpty()) {
        m_columns.first()->focusQuickAdd();
        m_boardScroll->ensureWidgetVisible(m_columns.first());
    }
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

    auto* column = columnContaining(id);
    if (!column) return false;
    column->selectTopicId(id);
    m_boardScroll->ensureWidgetVisible(column);
    m_selectedTopicId = id;
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
    {
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
    int count = 0;
    for (const auto* column : m_columns) count += column->itemCount();
    return count;
}

void StudyPage::onTopicsChanged() {
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
    m_inReset = false;
    const auto selection = selectedTopicId();
    if (!selection) {
        if (m_selectedTopicId) {
            m_selectedTopicId.reset();
            m_detail->clearTopic();
        }
        return;
    }
    if (m_selectedTopicId == *selection) {
        m_detail->refreshFromRepository();
    } else {
        m_selectedTopicId = *selection;
        m_detail->loadFrom(*selection);
    }
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
    const int from = orderedIds.indexOf(id);
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
    qCWarning(lcData) << what << error;
    m_model->reload();
    showMessage(error.isEmpty() ? what : u"%1 %2"_s.arg(what, error), "error");
}

void StudyPage::onCurrentChanged(const QModelIndex& current) {
    if (m_inReset) return;
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
    if (column->itemCount() == 0) return;
    const int clamped = std::clamp(row, 0, column->itemCount() - 1);
    column->list()->setCurrentIndex(column->proxy()->index(clamped, 0));
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
    for (const auto* column : m_columns) {
        const QModelIndex current = column->list()->currentIndex();
        if (current.isValid()) return current.data(Models::TopicListModel::IdRole).toInt();
    }
    return std::nullopt;
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
    const int total = m_model->rowCount();
    if (total == 0) {
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
