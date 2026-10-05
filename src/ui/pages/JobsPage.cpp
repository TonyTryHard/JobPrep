#include "ui/pages/JobsPage.h"

#include <QAction>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>
#include "data/ApplicationRepository.h"
#include "data/DbFormat.h"
#include "data/InterviewRepository.h"
#include "domain/EnumStrings.h"
#include "models/ApplicationFilterProxy.h"
#include "models/ApplicationTableModel.h"
#include "services/ExportService.h"
#include "ui/delegates/StatusBadgeDelegate.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/theme/StatusStyle.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/EmptyState.h"

using namespace Qt::StringLiterals;
using JobPrep::Domain::ApplicationStatus;

namespace JobPrep::Ui::Pages {

namespace {

constexpr int kViewNoData = 0;
constexpr int kViewTable = 1;
constexpr int kViewNoMatches = 2;

/// Statuses offered in the context menu submenu, in pipeline order. The footer reuses
/// it so a status is never forgotten in one place and shown in the other.
const ApplicationStatus kStatusChoices[] = {
    ApplicationStatus::Wishlist, ApplicationStatus::Applied,  ApplicationStatus::HrScreen,
    ApplicationStatus::Technical, ApplicationStatus::Final,    ApplicationStatus::Offer,
    ApplicationStatus::Accepted,  ApplicationStatus::Rejected, ApplicationStatus::Withdrawn,
    ApplicationStatus::Ghosted};

/// "Open posting URL" is only meaningful for a real http(s) link.
bool isOpenableUrl(QStringView url) {
    const QUrl parsed{url.toString()};
    const QString scheme = parsed.scheme().toLower();
    return (scheme == u"http"_s || scheme == u"https"_s) && !parsed.host().isEmpty();
}

QString salaryText(const JobPrep::Domain::JobApplication& app) {
    if (app.salaryMin.has_value() && app.salaryMax.has_value()) {
        return QString::number(*app.salaryMin) + u"\u2013"_s + QString::number(*app.salaryMax) +
               u' ' + app.currency;
    }
    if (app.salaryMax.has_value()) return QString::number(*app.salaryMax) + u' ' + app.currency;
    if (app.salaryMin.has_value()) return QString::number(*app.salaryMin) + u' ' + app.currency;
    return QString();
}

}  // namespace

JobsPage::JobsPage(JobPrep::Data::ApplicationRepository& apps,
                   JobPrep::Data::InterviewRepository& interviews,
                   JobPrep::Services::ExportService& exportService,
                   QWidget* parent)
    : PageBase(tr("Job Applications"), parent),
      m_apps(apps),
      m_interviews(interviews),
      m_exportService(exportService) {
    m_model = new Models::ApplicationTableModel(apps, interviews, this);
    m_proxy = new Models::ApplicationFilterProxy(this);
    m_proxy->setSourceModel(m_model);

    setupUi(new QVBoxLayout(this));

    connect(m_proxy, &QAbstractItemModel::modelReset, this, &JobsPage::onRowsChanged);
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, &JobsPage::onRowsChanged);
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, &JobsPage::onRowsChanged);
    refreshViewState();
}

void JobsPage::setupUi(QVBoxLayout* layout) {
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(16);

    setupToolbar(layout);
    setupTable(layout);
    setupEmptyStates();
    setupContextMenu();

    m_countsLabel = new QLabel(this);
    m_countsLabel->setObjectName(u"JobsFooterCounts"_s);
    m_countsLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_countsLabel);
}

void JobsPage::setupToolbar(QVBoxLayout* layout) {
    auto* bar = new QWidget(this);
    bar->setObjectName(u"JobsToolbar"_s);
    auto* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(16);

    m_searchEdit = new QLineEdit(bar);
    m_searchEdit->setObjectName(u"JobsSearchEdit"_s);
    m_searchEdit->setPlaceholderText(tr("Search company, position, notes"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(320);
    barLayout->addWidget(m_searchEdit, 0);

    m_chips = new Widgets::ChipBar(bar);
    m_chips->setObjectName(u"JobsStatusChips"_s);
    m_chips->addChip(tr("All"), Models::ApplicationFilterProxy::All);
    m_chips->addChip(tr("Active"), Models::ApplicationFilterProxy::Active);
    m_chips->addChip(tr("Interviewing"), Models::ApplicationFilterProxy::Interviewing);
    m_chips->addChip(tr("Offer"), Models::ApplicationFilterProxy::Offer);
    m_chips->addChip(tr("Closed"), Models::ApplicationFilterProxy::Closed);
    m_chips->setCurrent(Models::ApplicationFilterProxy::All);
    barLayout->addWidget(m_chips, 0);

    barLayout->addStretch(1);

    auto* exportBtn = new QPushButton(tr("Export CSV"), bar);
    exportBtn->setObjectName(u"JobsExportBtn"_s);
    exportBtn->setToolTip(tr("Export the rows you can see to a CSV file"));
    exportBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(exportBtn, 0);

    auto* addBtn = new QPushButton(tr("+ Application"), bar);
    addBtn->setObjectName(u"JobsAddBtn"_s);
    addBtn->setProperty("primary", true);
    addBtn->setToolTip(tr("Add a job application (Ctrl+N)"));
    addBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(addBtn, 0);

    layout->addWidget(bar);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &JobsPage::setSearchText);
    connect(m_chips, &Widgets::ChipBar::chipSelected, this, [this](int chip) {
        m_proxy->setFilterChip(static_cast<Models::ApplicationFilterProxy::FilterChip>(chip));
        refreshViewState();
    });
    connect(addBtn, &QPushButton::clicked, this, &JobsPage::newApplication);
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        const QString path =
            QFileDialog::getSaveFileName(this, tr("Export CSV"), QString(), tr("CSV files (*.csv)"));
        if (path.isEmpty()) return;
        QString error;
        int exportedRows = 0;
        if (!exportTo(path, &exportedRows, &error)) {
            QMessageBox::warning(this, tr("Export failed"), error);
        }
    });
}

void JobsPage::setupTable(QVBoxLayout* layout) {
    m_view = new QTableView(this);
    m_view->setObjectName(u"JobsTableView"_s);
    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSortingEnabled(true);
    // SPEC §3.3: freshest first, so the table shows what changed most recently.
    m_view->sortByColumn(Models::ApplicationTableModel::UpdatedColumn, Qt::DescendingOrder);
    m_view->setWordWrap(false);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setHighlightSections(false);
    m_view->installEventFilter(this);

    // Only the status column is painted; every other column keeps the default delegate.
    m_view->setItemDelegateForColumn(Models::ApplicationTableModel::StatusColumn,
                                     new Ui::Delegates::StatusBadgeDelegate(m_view));

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new QWidget(this));
    m_stack->addWidget(m_view);
    layout->addWidget(m_stack, 1);

    connect(m_view, &QTableView::customContextMenuRequested, this,
            &JobsPage::onContextMenuRequested);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current) {
                m_selectedId = current.isValid()
                                   ? std::optional<int>(
                                         current.data(Models::ApplicationTableModel::IdRole).toInt())
                                   : std::nullopt;
            });
    const auto openEditor = [this](const QModelIndex& index) {
        if (!index.isValid()) return;
        editApplication(index.data(Models::ApplicationTableModel::IdRole).toInt());
    };
    connect(m_view, &QTableView::activated, this, openEditor);
    connect(m_view, &QTableView::doubleClicked, this, openEditor);
}

void JobsPage::setupEmptyStates() {
    auto* noDataState = new Widgets::EmptyState(
        u"jobs"_s, tr("No Applications Yet"),
        tr("Track job openings, manage interview stages, and follow up with recruiters."),
        tr("+ Add application"), m_stack);
    connect(noDataState, &Widgets::EmptyState::actionClicked, this, &JobsPage::newApplication);
    m_stack->addWidget(noDataState);

    auto* noMatchesState = new Widgets::EmptyState(
        u"search"_s, tr("No Matches"),
        tr("No application matches the current search and status filter."),
        tr("Clear filters"), m_stack);
    connect(noMatchesState, &Widgets::EmptyState::actionClicked, this, [this]() {
        const QSignalBlocker blocker(m_searchEdit);
        m_searchEdit->clear();
        setSearchText(QString());
        m_chips->setCurrent(Models::ApplicationFilterProxy::All);
        m_proxy->setFilterChip(Models::ApplicationFilterProxy::All);
        refreshViewState();
    });
    m_stack->addWidget(noMatchesState);
}

void JobsPage::setupContextMenu() {
    m_contextMenu = new QMenu(this);
    m_contextMenu->setObjectName(u"JobsContextMenu"_s);
    m_contextMenu->addAction(tr("Edit..."), this, [this]() {
        if (const auto id = selectedId()) editApplication(*id);
    });

    auto* statusMenu = m_contextMenu->addMenu(tr("Change status"));
    statusMenu->setObjectName(u"JobsStatusMenu"_s);
    for (const ApplicationStatus status : kStatusChoices) {
        statusMenu->addAction(Theme::StatusStyle::label(status), this, [this, status]() {
            if (const auto id = selectedId()) changeStatus(*id, status);
        });
    }

    // Interviews are a separate entity and arrive with the calendar page in M3.
    QAction* addInterview = m_contextMenu->addAction(tr("Add interview..."));
    addInterview->setEnabled(false);
    addInterview->setToolTip(tr("Arrives in M3"));

    QAction* openUrl = m_contextMenu->addAction(tr("Open posting URL"));
    openUrl->setToolTip(tr("Only http and https links can be opened"));
    connect(openUrl, &QAction::triggered, this, [this]() {
        const auto id = selectedId();
        if (!id) return;
        const auto stored = m_apps.byId(*id);
        if (!stored || !isOpenableUrl(stored->url)) return;
        QDesktopServices::openUrl(QUrl(stored->url));
    });

    m_contextMenu->addSeparator();
    m_contextMenu->addAction(tr("Duplicate"), this, [this]() {
        const auto id = selectedId();
        if (!id) return;
        JobPrep::Domain::JobApplication copy;
        if (m_apps.duplicate(*id, copy)) selectId(copy.id);
    });
    m_contextMenu->addAction(tr("Delete..."), this, [this]() {
        if (const auto id = selectedId()) deleteApplication(*id);
    });
}

bool JobsPage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_view && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        const auto id = selectedId();
        if (id.has_value() && key->key() == Qt::Key_Return && key->modifiers() == Qt::NoModifier) {
            editApplication(*id);
            return true;
        }
        if (id.has_value() && key->key() == Qt::Key_Delete) {
            deleteApplication(*id);
            return true;
        }
    }
    return PageBase::eventFilter(watched, event);
}

void JobsPage::focusSearch() {
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void JobsPage::triggerNew() {
    newApplication();
}

void JobsPage::setSearchText(const QString& text) {
    if (m_searchText == text) return;
    m_searchText = text;
    // Mirroring the header box under a blocker stops the two from echoing each other.
    const QSignalBlocker blocker(m_searchEdit);
    m_searchEdit->setText(text);
    m_proxy->setSearchText(text);
    refreshViewState();
}

QString JobsPage::searchText() const {
    return m_searchText;
}

int JobsPage::currentViewIndex() const {
    return m_stack->currentIndex();
}

int JobsPage::visibleRowCount() const {
    return m_proxy->rowCount();
}

void JobsPage::refreshViewState() {
    if (m_apps.isEmpty()) {
        m_stack->setCurrentIndex(kViewNoData);
    } else if (visibleRowCount() == 0) {
        m_stack->setCurrentIndex(kViewNoMatches);
    } else {
        m_stack->setCurrentIndex(kViewTable);
    }
    refreshFooterCounts();
}

void JobsPage::refreshFooterCounts() {
    // Counts cover every application, not only the visible rows (SPEC §3.3 A7).
    const auto counts = m_apps.countsByStatus();
    QStringList parts;
    parts.append(tr("%n application(s)", nullptr, m_apps.count()));
    for (const ApplicationStatus status : kStatusChoices) {
        const int count = counts.value(status, 0);
        if (count == 0) continue;
        parts.append(u"%1: %2"_s.arg(Theme::StatusStyle::label(status), QString::number(count)));
    }
    m_countsLabel->setText(parts.join(u"   \u00b7   "_s));
}

void JobsPage::onRowsChanged() {
    refreshViewState();
    restoreSelection();
}

void JobsPage::onContextMenuRequested(const QPoint& position) {
    const QModelIndex index = m_view->indexAt(position);
    if (!index.isValid()) return;
    // Right-clicking another row acts on that row, not on the current selection.
    if (index.data(Models::ApplicationTableModel::IdRole).toInt() !=
        selectedId().value_or(0)) {
        m_view->setCurrentIndex(index);
    }
    m_contextMenu->popup(m_view->viewport()->mapToGlobal(position));
}

std::optional<int> JobsPage::selectedId() const {
    const QModelIndex current = m_view->currentIndex();
    if (!current.isValid()) return std::nullopt;
    return current.data(Models::ApplicationTableModel::IdRole).toInt();
}

void JobsPage::selectId(int id) {
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        const QModelIndex index = m_proxy->index(row, Models::ApplicationTableModel::CompanyColumn);
        if (index.data(Models::ApplicationTableModel::IdRole).toInt() != id) continue;
        m_view->setCurrentIndex(m_proxy->index(row, Models::ApplicationTableModel::CompanyColumn));
        m_view->scrollTo(m_view->currentIndex(), QAbstractItemView::EnsureVisible);
        m_selectedId = id;
        return;
    }
    m_view->clearSelection();
    m_selectedId = std::nullopt;
}

void JobsPage::restoreSelection() {
    if (m_selectedId.has_value()) selectId(*m_selectedId);
}

void JobsPage::newApplication() {
    Ui::Dialogs::ApplicationDialog dialog(m_apps, m_interviews,
                                          Ui::Dialogs::ApplicationDialog::Create, this);
    if (dialog.exec() != QDialog::Accepted) return;
    selectId(dialog.application().id);
}

void JobsPage::editApplication(int id) {
    const auto stored = m_apps.byId(id);
    if (!stored) return;
    Ui::Dialogs::ApplicationDialog dialog(m_apps, m_interviews,
                                          Ui::Dialogs::ApplicationDialog::Edit, this);
    dialog.setApplication(*stored);
    if (dialog.exec() != QDialog::Accepted) return;
    selectId(id);
}

void JobsPage::deleteApplication(int id) {
    const auto stored = m_apps.byId(id);
    if (!stored) return;
    // SPEC §8: destructive actions always confirm, because interviews and the timeline
    // cascade away with the row.
    const auto answer =
        QMessageBox::question(this, tr("Delete application"),
                              tr("Delete the application at %1? Its interviews and timeline go "
                                 "too.")
                                  .arg(stored->company),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    if (!m_apps.remove(id)) {
        QMessageBox::warning(this, tr("Delete failed"), m_apps.lastError());
        return;
    }
    m_selectedId = std::nullopt;
    refreshViewState();
}

void JobsPage::changeStatus(int id, ApplicationStatus status) {
    if (!m_apps.setStatus(id, status)) {
        QMessageBox::warning(this, tr("Status change failed"), m_apps.lastError());
        return;
    }
    selectId(id);
}

QList<JobPrep::Services::ExportRow> JobsPage::buildExportRows() const {
    QList<JobPrep::Services::ExportRow> rows;
    rows.reserve(m_proxy->rowCount());
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        const auto at = [this, row](Models::ApplicationTableModel::Column column) {
            return m_proxy->index(row, column);
        };
        const int id = at(Models::ApplicationTableModel::CompanyColumn)
                           .data(Models::ApplicationTableModel::IdRole)
                           .toInt();
        const auto stored = m_apps.byId(id);
        if (!stored) continue;
        const auto status = at(Models::ApplicationTableModel::StatusColumn)
                                .data(Models::ApplicationTableModel::StatusRole)
                                .value<ApplicationStatus>();

        JobPrep::Services::ExportRow out;
        out.company = stored->company;
        out.position = stored->position;
        out.statusLabel = Theme::StatusStyle::label(status);
        out.applied = stored->appliedDate.has_value()
                          ? JobPrep::Data::DbFormat::toText(*stored->appliedDate)
                          : QString();
        out.nextStepText =
            at(Models::ApplicationTableModel::NextStepColumn).data(Qt::DisplayRole).toString();
        out.salaryText = salaryText(*stored);
        out.source = stored->source;
        out.updated = JobPrep::Data::DbFormat::toText(stored->updatedAt.date());
        out.url = stored->url;
        out.resumeVersion = stored->resumeVersion;
        out.location = stored->location;
        out.workModeLabel = JobPrep::Domain::EnumStrings::toString(stored->workMode);
        out.salaryMin = stored->salaryMin.has_value() ? QString::number(*stored->salaryMin) : QString();
        out.salaryMax = stored->salaryMax.has_value() ? QString::number(*stored->salaryMax) : QString();
        out.currency = stored->currency;
        out.nextAction = stored->nextAction;
        out.nextActionDate = stored->nextActionDate.has_value()
                                 ? JobPrep::Data::DbFormat::toText(*stored->nextActionDate)
                                 : QString();
        out.contactName = stored->contactName;
        out.contactEmail = stored->contactEmail;
        out.notes = stored->notes;
        out.createdAt = JobPrep::Data::DbFormat::toText(stored->createdAt.date());
        out.updatedAt = JobPrep::Data::DbFormat::toText(stored->updatedAt.date());
        rows.append(out);
    }
    return rows;
}

bool JobsPage::exportTo(const QString& path, int* exportedRows, QString* error) {
    const auto rows = buildExportRows();
    if (exportedRows) *exportedRows = rows.size();
    return m_exportService.exportToFile(rows, JobPrep::Services::ExportService::headers(), path,
                                        error);
}

}  // namespace JobPrep::Ui::Pages