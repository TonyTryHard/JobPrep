#include "ui/widgets/TopicDetailPanel.h"

#include <QApplication>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QDesktopServices>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QStyle>
#include <QTableWidget>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include "data/TopicRepository.h"
#include "ui/StudyLogging.h"
#include "ui/theme/StatusStyle.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/SingleTabDateTime.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

namespace {

constexpr int kResourceColumnCount = 2;
const int kSubtaskRole = Qt::UserRole;

/// Blocks refreshFromRepository for the duration of a panel-initiated repository write.
class InternalWriteGuard {
public:
    explicit InternalWriteGuard(bool& flag) : m_flag(flag) { m_flag = true; }
    ~InternalWriteGuard() { m_flag = false; }

private:
    bool& m_flag;
};

bool isHttpOrHttps(const QUrl& url) {
    if (!url.isValid()) return false;
    const QString scheme = url.scheme();
    return scheme.compare(u"http"_s, Qt::CaseInsensitive) == 0 ||
           scheme.compare(u"https"_s, Qt::CaseInsensitive) == 0;
}

}  // namespace

TopicDetailPanel::TopicDetailPanel(Data::TopicRepository& topics, Theme::ThemeManager& theme,
                                   QWidget* parent, OpenUrlOpener opener)
    : QWidget(parent),
      m_topics(topics),
      m_theme(theme),
      m_opener(std::move(opener)) {
    if (!m_opener) {
        m_opener = [](const QUrl& url) { return QDesktopServices::openUrl(url); };
    }
    setObjectName(u"StudyDetailPanel"_s);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* card = new QWidget(this);
    card->setObjectName(u"StudyDetailPanelCard"_s);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);

    auto* header = new QHBoxLayout;
    m_headingLabel = new QLabel(card);
    m_headingLabel->setObjectName(u"StudyDetailHeading"_s);
    m_headingLabel->setText(tr("Topic details"));
    header->addWidget(m_headingLabel, 1);

    m_closeBtn = new QPushButton(card);
    m_closeBtn->setObjectName(u"StudyDetailCloseBtn"_s);
    m_closeBtn->setToolTip(tr("Close the details (Esc)"));
    m_closeBtn->setAccessibleName(tr("Close details"));
    m_closeBtn->setFixedSize(28, 28);
    header->addWidget(m_closeBtn, 0, Qt::AlignTop);
    cardLayout->addLayout(header);

    m_messageLabel = new QLabel(card);
    m_messageLabel->setObjectName(u"StudyDetailMessageLabel"_s);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->hide();
    cardLayout->addWidget(m_messageLabel);

    m_scroll = new QScrollArea(card);
    m_scroll->setObjectName(u"StudyDetailScroll"_s);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_form = new QWidget(m_scroll);
    auto* form = new QVBoxLayout(m_form);
    form->setContentsMargins(0, 0, 8, 0);
    form->setSpacing(10);

    m_titleEdit = new QLineEdit(m_form);
    m_titleEdit->setObjectName(u"StudyDetailTitleEdit"_s);
    m_titleEdit->setPlaceholderText(tr("Topic title"));
    form->addWidget(m_titleEdit);

    auto* trackRow = new QHBoxLayout;
    m_trackCombo = new QComboBox(m_form);
    m_trackCombo->setObjectName(u"StudyDetailTrackCombo"_s);
    m_trackCombo->setToolTip(tr("Track this topic belongs to"));
    trackRow->addWidget(m_trackCombo, 1);
    form->addLayout(trackRow);

    auto* statusRow = new QHBoxLayout;
    m_statusCombo = new QComboBox(m_form);
    m_statusCombo->setObjectName(u"StudyDetailStatusCombo"_s);
    for (const auto status : {Domain::TopicStatus::Backlog, Domain::TopicStatus::InProgress,
                              Domain::TopicStatus::Review, Domain::TopicStatus::Done}) {
        m_statusCombo->addItem(Theme::StatusStyle::label(status), static_cast<int>(status));
    }
    m_priorityCombo = new QComboBox(m_form);
    m_priorityCombo->setObjectName(u"StudyDetailPriorityCombo"_s);
    for (const auto priority : {Domain::Priority::Low, Domain::Priority::Normal,
                                Domain::Priority::High}) {
        m_priorityCombo->addItem(Theme::StatusStyle::label(priority), static_cast<int>(priority));
    }
    statusRow->addWidget(m_statusCombo, 1);
    statusRow->addWidget(m_priorityCombo, 1);
    form->addLayout(statusRow);

    auto* dateRow = new QHBoxLayout;
    m_dateEdit = new SingleTabDateEdit(m_form);
    m_dateEdit->setObjectName(u"StudyDetailDateEdit"_s);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat(u"yyyy-MM-dd"_s);
    m_dateEdit->setSpecialValueText(tr("No target date"));
    m_dateEdit->setDate(m_dateEdit->minimumDate());
    if (auto* cal = m_dateEdit->calendarWidget()) {
        cal->installEventFilter(this);
    }
    m_dateBtn = new QPushButton(tr("Clear"), m_form);
    m_dateBtn->setObjectName(u"StudyDetailDateClearBtn"_s);
    m_dateBtn->setToolTip(tr("Clear the target date"));
    m_dateBtn->setEnabled(false);
    dateRow->addWidget(m_dateEdit, 1);
    dateRow->addWidget(m_dateBtn, 0);
    form->addLayout(dateRow);

    m_tagsEdit = new QLineEdit(m_form);
    m_tagsEdit->setObjectName(u"StudyDetailTagsEdit"_s);
    m_tagsEdit->setPlaceholderText(tr("Tags, comma separated"));
    form->addWidget(m_tagsEdit);

    m_notesEdit = new QTextEdit(m_form);
    m_notesEdit->setObjectName(u"StudyDetailNotesEdit"_s);
    m_notesEdit->setPlaceholderText(tr("Notes"));
    m_notesEdit->setAcceptRichText(false);
    m_notesEdit->setTabChangesFocus(true);
    m_notesEdit->setMinimumHeight(80);
    form->addWidget(m_notesEdit);

    auto* checkSection = new QLabel(tr("Checklist"), m_form);
    checkSection->setObjectName(u"StudyDetailSectionLabel"_s);
    form->addWidget(checkSection);

    m_checkList = new QListWidget(m_form);
    m_checkList->setObjectName(u"StudyDetailCheckList"_s);
    m_checkList->setDragDropMode(QAbstractItemView::InternalMove);
    m_checkList->setEditTriggers(QAbstractItemView::DoubleClicked |
                                 QAbstractItemView::EditKeyPressed);
    const int rowHeight = m_checkList->fontMetrics().lineSpacing() + 8;
    const int fiveRowsHeight = rowHeight * 5 + 4;
    m_checkList->setMinimumHeight(fiveRowsHeight);
    m_checkList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_checkList->installEventFilter(this);
    form->addWidget(m_checkList, 1);

    auto* checkAddRow = new QHBoxLayout;
    m_checkAddEdit = new QLineEdit(m_form);
    m_checkAddEdit->setObjectName(u"StudyDetailCheckAddEdit"_s);
    m_checkAddEdit->setPlaceholderText(tr("New checklist item"));
    auto* checkAddBtn = new QPushButton(tr("Add"), m_form);
    checkAddBtn->setObjectName(u"StudyDetailCheckAddBtn"_s);
    checkAddBtn->setAutoDefault(false);
    checkAddBtn->setDefault(false);
    checkAddBtn->setFocusPolicy(Qt::ClickFocus);
    checkAddRow->addWidget(m_checkAddEdit, 1);
    checkAddRow->addWidget(checkAddBtn, 0);
    form->addLayout(checkAddRow);

    auto* resourcesSection = new QLabel(tr("Resources"), m_form);
    resourcesSection->setObjectName(u"StudyDetailSectionLabel"_s);
    form->addWidget(resourcesSection);

    m_resourcesTable = new QTableWidget(m_form);
    m_resourcesTable->setObjectName(u"StudyDetailResourcesTable"_s);
    m_resourcesTable->setColumnCount(kResourceColumnCount);
    m_resourcesTable->setHorizontalHeaderLabels({tr("Title"), tr("URL")});
    m_resourcesTable->horizontalHeader()->setStretchLastSection(true);
    m_resourcesTable->verticalHeader()->hide();
    m_resourcesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resourcesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resourcesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resourcesTable->setMinimumHeight(120);
    form->addWidget(m_resourcesTable);

    auto* resourceAddRow = new QHBoxLayout;
    m_resourceTitleEdit = new QLineEdit(m_form);
    m_resourceTitleEdit->setObjectName(u"StudyDetailResourceTitleEdit"_s);
    m_resourceTitleEdit->setPlaceholderText(tr("Resource title"));
    m_resourceUrlEdit = new QLineEdit(m_form);
    m_resourceUrlEdit->setObjectName(u"StudyDetailResourceUrlEdit"_s);
    m_resourceUrlEdit->setPlaceholderText(tr("https://..."));
    auto* resourceAddBtn = new QPushButton(tr("Add"), m_form);
    resourceAddBtn->setObjectName(u"StudyDetailResourceAddBtn"_s);
    resourceAddRow->addWidget(m_resourceTitleEdit, 1);
    resourceAddRow->addWidget(m_resourceUrlEdit, 2);
    resourceAddRow->addWidget(resourceAddBtn, 0);
    form->addLayout(resourceAddRow);

    auto* resourceButtonsRow = new QHBoxLayout;
    m_resourceOpenBtn = new QPushButton(tr("Open"), m_form);
    m_resourceOpenBtn->setObjectName(u"StudyDetailResourceOpenBtn"_s);
    m_resourceOpenBtn->setToolTip(tr("Only http and https links can be opened"));
    m_resourceDeleteBtn = new QPushButton(tr("Delete"), m_form);
    m_resourceDeleteBtn->setObjectName(u"StudyDetailResourceDeleteBtn"_s);
    resourceButtonsRow->addWidget(m_resourceOpenBtn);
    resourceButtonsRow->addWidget(m_resourceDeleteBtn);
    resourceButtonsRow->addStretch(1);
    form->addLayout(resourceButtonsRow);

    m_logTimeBtn = new QPushButton(tr("Log time"), m_form);
    m_logTimeBtn->setObjectName(u"StudyDetailLogTimeBtn"_s);
    m_logTimeBtn->setToolTip(tr("Log study session for this topic"));
    connect(m_logTimeBtn, &QPushButton::clicked, this, [this]() {
        if (m_topicId.has_value() && *m_topicId > 0) {
            emit logTimeRequested(*m_topicId);
        }
    });
    form->addWidget(m_logTimeBtn, 0, Qt::AlignLeft);

    setTabOrder(m_titleEdit, m_trackCombo);
    setTabOrder(m_trackCombo, m_statusCombo);
    setTabOrder(m_statusCombo, m_priorityCombo);
    setTabOrder(m_priorityCombo, m_dateEdit);
    setTabOrder(m_dateEdit, m_dateBtn);
    setTabOrder(m_dateBtn, m_tagsEdit);
    setTabOrder(m_tagsEdit, m_notesEdit);
    setTabOrder(m_notesEdit, m_checkList);
    setTabOrder(m_checkList, m_checkAddEdit);
    setTabOrder(m_checkAddEdit, m_resourcesTable);
    setTabOrder(m_resourcesTable, m_resourceTitleEdit);
    setTabOrder(m_resourceTitleEdit, m_resourceUrlEdit);
    setTabOrder(m_resourceUrlEdit, resourceAddBtn);
    setTabOrder(resourceAddBtn, m_resourceOpenBtn);
    setTabOrder(m_resourceOpenBtn, m_resourceDeleteBtn);
    setTabOrder(m_resourceDeleteBtn, m_logTimeBtn);
    setTabOrder(m_logTimeBtn, m_closeBtn);

    m_scroll->setWidget(m_form);
    cardLayout->addWidget(m_scroll, 1);
    layout->addWidget(card, 1);

    setFormEnabled(false);
    updateResourceButtons();

    const auto editors = {static_cast<QObject*>(m_titleEdit), static_cast<QObject*>(m_trackCombo),
                          static_cast<QObject*>(m_statusCombo),
                          static_cast<QObject*>(m_priorityCombo), static_cast<QObject*>(m_dateEdit),
                          static_cast<QObject*>(m_tagsEdit), static_cast<QObject*>(m_notesEdit),
                          static_cast<QObject*>(m_checkAddEdit)};
    for (QObject* editor : editors) {
        editor->installEventFilter(this);
        if (auto* widget = qobject_cast<QWidget*>(editor)) {
            widget->setFocusPolicy(Qt::StrongFocus);
        }
    }
    connect(m_titleEdit, &QLineEdit::textChanged, this, &TopicDetailPanel::markDirty);
    connect(m_trackCombo, &QComboBox::currentIndexChanged, this, &TopicDetailPanel::markDirty);
    connect(m_statusCombo, &QComboBox::currentIndexChanged, this, &TopicDetailPanel::markDirty);
    connect(m_priorityCombo, &QComboBox::currentIndexChanged, this,
            &TopicDetailPanel::markDirty);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](const QDate& d) {
        if (m_loading) return;
        if (d != m_dateEdit->minimumDate()) {
            m_hasTargetDate = true;
            updateDateControls();
            markDirty();
            flush();
        }
    });
    connect(m_tagsEdit, &QLineEdit::textChanged, this, &TopicDetailPanel::markDirty);
    connect(m_notesEdit, &QTextEdit::textChanged, this, &TopicDetailPanel::markDirty);
    connect(m_dateBtn, &QPushButton::clicked, this, &TopicDetailPanel::clearTargetDate);
    connect(checkAddBtn, &QPushButton::clicked, this, &TopicDetailPanel::addChecklistItem);
    connect(m_checkAddEdit, &QLineEdit::returnPressed, this,
            &TopicDetailPanel::addChecklistItem);
    connect(m_checkList, &QListWidget::itemChanged, this,
            &TopicDetailPanel::onChecklistItemChanged);
    connect(m_checkList->model(), &QAbstractItemModel::rowsInserted, this,
            [this] { if (!m_syncingChecklist) persistChecklistOrder(); });
    connect(m_checkList->model(), &QAbstractItemModel::rowsRemoved, this,
            [this] { if (!m_syncingChecklist) persistChecklistOrder(); });
    connect(&m_theme, &Theme::ThemeManager::themeChanged, this, [this]() {
        if (m_checkList) m_checkList->viewport()->update();
        if (m_resourcesTable) m_resourcesTable->viewport()->update();
    });
    connect(resourceAddBtn, &QPushButton::clicked, this, &TopicDetailPanel::addResource);
    connect(m_resourceUrlEdit, &QLineEdit::returnPressed, this, &TopicDetailPanel::addResource);
    connect(m_resourceOpenBtn, &QPushButton::clicked, this, &TopicDetailPanel::openResource);
    connect(m_resourceDeleteBtn, &QPushButton::clicked, this, &TopicDetailPanel::deleteResource);
    connect(m_resourcesTable, &QTableWidget::itemSelectionChanged, this,
            &TopicDetailPanel::updateResourceButtons);
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        flush();
        emit closeRequested();
    });

    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(saveShortcut, &QShortcut::activated, this, [this]() { flush(); });
}

void TopicDetailPanel::setTracks(const QList<Domain::Track>& tracks) {
    const QSignalBlocker blocker(m_trackCombo);
    const int previous = m_trackCombo->currentData().toInt();
    m_trackCombo->clear();
    for (const auto& track : tracks) {
        m_trackCombo->addItem(track.name, track.id);
    }

    const int index = m_trackCombo->findData(previous);
    if (index >= 0) {
        m_trackCombo->setCurrentIndex(index);
    } else if (m_trackCombo->count() > 0) {
        m_trackCombo->setCurrentIndex(0);
    }
}

bool TopicDetailPanel::loadFrom(int topicId) {
    qCDebug(lcStudy) << "panel loadFrom:" << topicId;
    if (m_topicId == topicId) {
        if (!m_dirty) {
            const auto topic = m_topics.byId(topicId);
            if (topic) {
                loadEditors(*topic);
                return true;
            }
            clearTopic();
            return false;
        }
        return true;
    }
    flush();
    const auto topic = m_topics.byId(topicId);
    if (!topic) {
        clearTopic();
        return false;
    }
    m_topicId = topicId;
    loadEditors(*topic);
    m_dirty = false;
    return true;
}

bool TopicDetailPanel::flush() {
    if (!m_topicId || !m_dirty) return true;
    return saveNow();
}

void TopicDetailPanel::clearTopic() {
    qCDebug(lcStudy) << "panel clearTopic";
    m_topicId.reset();
    m_dirty = false;
    m_loading = true;
    m_headingLabel->setText(tr("Select a topic"));
    m_titleEdit->clear();
    m_trackCombo->clear();
    m_statusCombo->setCurrentIndex(1);
    m_priorityCombo->setCurrentIndex(1);
    m_hasTargetDate = false;
    m_tagsEdit->clear();
    m_notesEdit->clear();
    m_checkList->clear();
    m_checkAddEdit->clear();
    m_resourcesTable->setRowCount(0);
    m_loading = false;
    showMessage(tr("Select a topic"), "info");
    setFormEnabled(false);
    updateDateControls();
    updateResourceButtons();
}

void TopicDetailPanel::refreshFromRepository() {
    if (!m_topicId) return;
    if (m_internalWrite) return;
    if (hasFocusInEditor()) return;
    if (m_dirty) return;
    const auto topic = m_topics.byId(*m_topicId);
    if (!topic) return;
    loadEditors(*topic);
}

bool TopicDetailPanel::hasFocusInEditor() const {
    const QWidget* focus = QApplication::focusWidget();
    return focus != nullptr && (focus == this || isAncestorOf(focus));
}

void TopicDetailPanel::setSessionLogEnabled(bool enabled) {
    m_logTimeBtn->setEnabled(enabled);
}

void TopicDetailPanel::focusFirstEditor() {
    if (m_titleEdit && m_titleEdit->isEnabled()) {
        m_titleEdit->setFocus(Qt::OtherFocusReason);
        m_titleEdit->selectAll();
    }
}

bool TopicDetailPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::FocusOut && m_topicId && !m_loading) {
        // Autosave when focus leaves an editor (B5). Guard against the focus-out of a
        // child popup (combo dropdown, calendar) by checking the new focus widget.
        const QWidget* next = QApplication::focusWidget();
        bool isChildPopup = false;
        if (next) {
            if (m_dateEdit && (next == m_dateEdit->calendarWidget() || m_dateEdit->calendarWidget()->isAncestorOf(next))) {
                isChildPopup = true;
            }
            if (m_trackCombo && next == m_trackCombo->view()) isChildPopup = true;
            if (m_statusCombo && next == m_statusCombo->view()) isChildPopup = true;
            if (m_priorityCombo && next == m_priorityCombo->view()) isChildPopup = true;
        }
        if (!isChildPopup) {
            flush();
        }
    }

    if (event->type() == QEvent::KeyPress && m_topicId) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (watched == m_dateEdit &&
            (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)) {
            clearTargetDate();
            return true;
        }

        // Esc collapses the panel from any editor (B1); QLineEdit would swallow it.
        if (keyEvent->key() == Qt::Key_Escape) {
            flush();
            emit closeRequested();
            return true;
        }

        if (watched == m_checkList && keyEvent->key() == Qt::Key_Space &&
            !keyEvent->modifiers().testFlag(Qt::AltModifier) &&
            !keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
            auto* item = m_checkList->currentItem();
            if (item && (item->flags() & Qt::ItemIsUserCheckable)) {
                item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
                return true;
            }
        }

        if (watched == m_checkList && keyEvent->modifiers().testFlag(Qt::AltModifier) &&
            (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)) {
            moveChecklistItem(keyEvent->key() == Qt::Key_Up ? -1 : 1);
            return true;
        }
        if (watched == m_checkList && keyEvent->key() == Qt::Key_Delete) {
            deleteChecklistItem();
            return true;
        }
    }

    if (m_dateEdit && watched == m_dateEdit->calendarWidget() && event->type() == QEvent::Show) {
        if (!m_hasTargetDate) {
            m_dateEdit->calendarWidget()->setSelectedDate(QDate::currentDate());
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TopicDetailPanel::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && m_topicId) {
        flush();
        emit closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void TopicDetailPanel::markDirty() {
    if (m_loading || !m_topicId) return;
    m_dirty = true;
}

bool TopicDetailPanel::saveNow() {
    if (!m_topicId || m_loading) return false;
    const auto topic = m_topics.byId(*m_topicId);
    if (!topic) {
        clearTopic();
        return false;
    }

    Domain::Topic updated = *topic;
    updated.title = m_titleEdit->text().trimmed();

    if (updated.title.isEmpty()) {
        const QSignalBlocker blocker(m_titleEdit);
        m_titleEdit->setText(topic->title);
        showMessage(tr("Title cannot be empty."), "error");
        return false;
    }
    if (m_trackCombo->currentIndex() < 0 || !m_trackCombo->currentData().isValid() || m_trackCombo->count() == 0) {
        loadEditors(*topic);
        m_dirty = true;
        showMessage(tr("Tracks changed. Please reselect."), "error");
        return false;
    }
    updated.trackId = m_trackCombo->currentData().toInt();
    updated.status = static_cast<Domain::TopicStatus>(m_statusCombo->currentData().toInt());
    updated.priority = static_cast<Domain::Priority>(m_priorityCombo->currentData().toInt());
    updated.targetDate =
        (m_hasTargetDate && m_dateEdit->date() != m_dateEdit->minimumDate())
            ? std::optional<QDate>(m_dateEdit->date())
            : std::nullopt;
    updated.notes = m_notesEdit->toPlainText();
    updated.tags = m_tagsEdit->text().trimmed();
    updated.resources = resourcesJson();

    const bool changed = (updated.title != topic->title ||
                          updated.trackId != topic->trackId ||
                          updated.status != topic->status ||
                          updated.priority != topic->priority ||
                          updated.targetDate != topic->targetDate ||
                          updated.notes != topic->notes ||
                          updated.tags != topic->tags ||
                          updated.resources != topic->resources);

    qCDebug(lcStudy) << "panel save: wrote=" << changed;

    if (!changed) {
        m_dirty = false;
        hideMessage();
        return true;
    }

    {
        const InternalWriteGuard guard(m_internalWrite);
        if (!m_topics.update(updated)) {
            emit writeFailed(m_topics.lastError());
            return false;
        }
    }
    m_dirty = false;
    hideMessage();
    return true;
}

void TopicDetailPanel::loadEditors(const Domain::Topic& topic) {
    m_loading = true;
    m_headingLabel->setText(topic.title.isEmpty() ? tr("Topic details") : topic.title);

    if (!m_titleEdit->hasFocus() || m_titleEdit->text() != topic.title) {
        const QSignalBlocker blocker(m_titleEdit);
        m_titleEdit->setText(topic.title);
    }
    {
        const QSignalBlocker blocker(m_trackCombo);
        int index = m_trackCombo->findData(topic.trackId);
        if (index < 0 && m_trackCombo->count() > 0) index = 0;
        if (index < 0 && !topic.trackId) index = -1;
        if (m_trackCombo->currentIndex() != index) {
            m_trackCombo->setCurrentIndex(index);
        }
    }
    {
        const QSignalBlocker blocker(m_statusCombo);
        const int idx = m_statusCombo->findData(static_cast<int>(topic.status));
        if (m_statusCombo->currentIndex() != idx) {
            m_statusCombo->setCurrentIndex(idx);
        }
    }
    {
        const QSignalBlocker blocker(m_priorityCombo);
        const int idx = m_priorityCombo->findData(static_cast<int>(topic.priority));
        if (m_priorityCombo->currentIndex() != idx) {
            m_priorityCombo->setCurrentIndex(idx);
        }
    }
    m_hasTargetDate = topic.targetDate.has_value();
    if (!m_dateEdit->hasFocus()) {
        const QSignalBlocker blocker(m_dateEdit);
        if (topic.targetDate) {
            m_dateEdit->setDate(*topic.targetDate);
        } else {
            m_dateEdit->setDate(m_dateEdit->minimumDate());
        }
    }
    if (!m_tagsEdit->hasFocus() || m_tagsEdit->text() != topic.tags) {
        const QSignalBlocker blocker(m_tagsEdit);
        m_tagsEdit->setText(topic.tags);
    }
    if (!m_notesEdit->hasFocus() || m_notesEdit->toPlainText() != topic.notes) {
        const QSignalBlocker blocker(m_notesEdit);
        m_notesEdit->setPlainText(topic.notes);
    }

    fillChecklist(topic.id);
    fillResources(topic.resources);
    m_loading = false;
    m_dirty = false;
    hideMessage();
    setFormEnabled(true);
    updateDateControls();
    updateResourceButtons();
}

void TopicDetailPanel::fillChecklist(int topicId) {
    const auto subtasks = m_topics.subtasks(topicId);
    if (m_checkList->count() == subtasks.size()) {
        bool matches = true;
        for (int i = 0; i < subtasks.size(); ++i) {
            const auto* item = m_checkList->item(i);
            const auto& st = subtasks.at(i);
            if (!item || item->data(kSubtaskRole).toInt() != st.id ||
                item->text() != st.text ||
                (item->checkState() == Qt::Checked) != st.done) {
                matches = false;
                break;
            }
        }
        if (matches) return;
    }

    m_syncingChecklist = true;
    m_checkList->clear();
    for (const auto& subtask : subtasks) {
        auto* item = new QListWidgetItem(subtask.text, m_checkList);
        item->setData(kSubtaskRole, subtask.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        item->setCheckState(subtask.done ? Qt::Checked : Qt::Unchecked);
    }
    m_syncingChecklist = false;

    qCDebug(lcStudy) << "checklist refreshed: count=" << m_checkList->count();
}

void TopicDetailPanel::fillResources(const QString& json) {
    m_resourcesTable->setRowCount(0);
    if (json.trimmed().isEmpty()) return;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) return;
    const QJsonArray array = document.array();
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        const int row = m_resourcesTable->rowCount();
        m_resourcesTable->insertRow(row);
        m_resourcesTable->setItem(row, 0, new QTableWidgetItem(object.value(u"title"_s).toString()));
        m_resourcesTable->setItem(row, 1, new QTableWidgetItem(object.value(u"url"_s).toString()));
    }
}

QString TopicDetailPanel::resourcesJson() const {
    QJsonArray array;
    for (int row = 0; row < m_resourcesTable->rowCount(); ++row) {
        const auto* titleItem = m_resourcesTable->item(row, 0);
        const auto* urlItem = m_resourcesTable->item(row, 1);
        QJsonObject object;
        object.insert(u"title"_s, titleItem ? titleItem->text() : QString());
        object.insert(u"url"_s, urlItem ? urlItem->text() : QString());
        array.append(object);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void TopicDetailPanel::persistChecklistOrder() {
    if (m_loading || !m_topicId) return;
    QList<int> orderedIds;
    orderedIds.reserve(m_checkList->count());
    for (int row = 0; row < m_checkList->count(); ++row) {
        orderedIds.append(m_checkList->item(row)->data(kSubtaskRole).toInt());
    }
    if (orderedIds.isEmpty()) return;
    const InternalWriteGuard guard(m_internalWrite);
    if (!m_topics.reorderSubtasks(*m_topicId, orderedIds)) {
        emit writeFailed(m_topics.lastError());
    }
}

void TopicDetailPanel::showMessage(const QString& text, const char* kind) {
    m_messageLabel->setText(text);
    m_messageLabel->setProperty("kind", QByteArray(kind));
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    m_messageLabel->show();
}

void TopicDetailPanel::hideMessage() {
    m_messageLabel->hide();
}

void TopicDetailPanel::updateResourceButtons() {
    const bool hasSelection = !m_resourcesTable->selectedItems().isEmpty();
    m_resourceOpenBtn->setEnabled(hasSelection);
    m_resourceDeleteBtn->setEnabled(hasSelection);
}

void TopicDetailPanel::updateDateControls() {
    const bool enabled = m_form->isEnabled();
    m_dateEdit->setEnabled(enabled);
    m_dateBtn->setText(tr("Clear"));
    m_dateBtn->setToolTip(tr("Clear the target date"));
    m_dateBtn->setEnabled(enabled && m_hasTargetDate);
    if (!m_hasTargetDate) {
        const QSignalBlocker blocker(m_dateEdit);
        m_dateEdit->setDate(m_dateEdit->minimumDate());
    }
}

void TopicDetailPanel::setFormEnabled(bool enabled) {
    m_form->setEnabled(enabled);
    m_headingLabel->setEnabled(enabled);
    if (!enabled) {
        m_headingLabel->setText(tr("Select a topic"));
        showMessage(tr("Select a topic"), "info");
    }
}

void TopicDetailPanel::onChecklistItemChanged(QListWidgetItem* item) {
    if (m_loading || !m_topicId || !item) return;
    const int subtaskId = item->data(kSubtaskRole).toInt();
    if (subtaskId <= 0) return;

    Domain::Subtask updated;
    updated.id = subtaskId;
    updated.topicId = *m_topicId;
    updated.text = item->text().trimmed();
    updated.done = item->checkState() == Qt::Checked;

    const auto subtasks = m_topics.subtasks(*m_topicId);
    for (const auto& existing : subtasks) {
        if (existing.id == subtaskId) {
            updated.position = existing.position;
            break;
        }
    }

    if (updated.text.isEmpty()) {
        m_syncingChecklist = true;
        for (const auto& existing : subtasks) {
            if (existing.id == subtaskId) {
                item->setText(existing.text);
                break;
            }
        }
        m_syncingChecklist = false;
        showMessage(tr("Checklist text cannot be empty."), "error");
        return;
    }

    const InternalWriteGuard guard(m_internalWrite);
    if (!m_topics.updateSubtask(updated)) {
        emit writeFailed(m_topics.lastError());
    }
}

void TopicDetailPanel::addChecklistItem() {
    const QString text = m_checkAddEdit->text().trimmed();
    qCDebug(lcStudy) << "checklist add: topic=" << (m_topicId ? *m_topicId : -1) << "len=" << text.length();
    if (!m_topicId) return;
    if (text.isEmpty()) return;

    Domain::Subtask subtask;
    subtask.topicId = *m_topicId;
    subtask.text = text;
    subtask.position = m_checkList->count();
    {
        const InternalWriteGuard guard(m_internalWrite);
        if (!m_topics.addSubtask(subtask)) {
            emit writeFailed(m_topics.lastError());
            return;
        }
    }

    fillChecklist(*m_topicId);
    if (m_checkList->count() > 0) {
        m_checkList->scrollToItem(m_checkList->item(m_checkList->count() - 1));
    }
    m_checkAddEdit->clear();
    m_checkAddEdit->setFocus();
}

void TopicDetailPanel::deleteChecklistItem() {
    if (!m_topicId) return;
    auto* item = m_checkList->currentItem();
    if (!item) return;
    const int subtaskId = item->data(kSubtaskRole).toInt();
    {
        const InternalWriteGuard guard(m_internalWrite);
        if (!m_topics.removeSubtask(subtaskId)) {
            emit writeFailed(m_topics.lastError());
            return;
        }
    }
    m_syncingChecklist = true;
    delete m_checkList->takeItem(m_checkList->row(item));
    m_syncingChecklist = false;
    persistChecklistOrder();
}

void TopicDetailPanel::moveChecklistItem(int delta) {
    const int row = m_checkList->currentRow();
    if (row < 0) return;
    const int target = row + delta;
    if (target < 0 || target >= m_checkList->count()) return;
    m_syncingChecklist = true;
    auto* item = m_checkList->takeItem(row);
    m_checkList->insertItem(target, item);
    m_checkList->setCurrentItem(item);
    m_syncingChecklist = false;
    persistChecklistOrder();
}

void TopicDetailPanel::addResource() {
    if (!m_topicId) return;
    const QString title = m_resourceTitleEdit->text().trimmed();
    const QString url = m_resourceUrlEdit->text().trimmed();
    if (title.isEmpty() && url.isEmpty()) return;

    if (!url.isEmpty()) {
        const QUrl parsed(url, QUrl::StrictMode);
        if (!isHttpOrHttps(parsed)) {
            showMessage(tr("Only http and https links can be added."), "error");
            return;
        }
    }

    const int row = m_resourcesTable->rowCount();
    m_resourcesTable->insertRow(row);
    m_resourcesTable->setItem(row, 0, new QTableWidgetItem(title));
    m_resourcesTable->setItem(row, 1, new QTableWidgetItem(url));
    m_resourceTitleEdit->clear();
    m_resourceUrlEdit->clear();
    markDirty();
    flush();
}

void TopicDetailPanel::openResource() {
    const auto* item = m_resourcesTable->currentItem();
    if (!item) return;
    const QString text = m_resourcesTable->item(item->row(), 1)->text().trimmed();
    const QUrl url(text, QUrl::StrictMode);
    if (!isHttpOrHttps(url)) {
        showMessage(tr("Only http and https links can be opened."), "error");
        return;
    }
    m_opener(url);
}

void TopicDetailPanel::deleteResource() {
    const int row = m_resourcesTable->currentRow();
    if (row < 0) return;
    m_resourcesTable->removeRow(row);
    markDirty();
    flush();
}

void TopicDetailPanel::clearTargetDate() {
    if (!m_topicId) return;
    m_hasTargetDate = false;
    updateDateControls();
    markDirty();
    flush();
}

}  // namespace JobPrep::Ui::Widgets
