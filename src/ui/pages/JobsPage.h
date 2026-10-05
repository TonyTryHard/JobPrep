#pragma once

#include <QHash>
#include <optional>
#include "domain/Enums.h"
#include "ui/PageBase.h"

class QLabel;
class QLineEdit;
class QMenu;
class QStackedWidget;
class QTableView;
class QVBoxLayout;

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
}

namespace JobPrep::Models {
class ApplicationFilterProxy;
class ApplicationTableModel;
}  // namespace JobPrep::Models

namespace JobPrep::Services {
class ExportService;
struct ExportRow;
}  // namespace JobPrep::Services

namespace JobPrep::Ui::Widgets {
class ChipBar;
class EmptyState;
}  // namespace JobPrep::Ui::Widgets

namespace JobPrep::Ui::Pages {

/// Job applications tracker (SPEC §3.3): a sortable, filterable table of applications
/// with a status chip bar, CSV export and a per-row context menu.
class JobsPage : public PageBase {
    Q_OBJECT

public:
    JobsPage(JobPrep::Data::ApplicationRepository& apps,
             JobPrep::Data::InterviewRepository& interviews,
             JobPrep::Services::ExportService& exportService,
             QWidget* parent = nullptr);

    void focusSearch() override;
    void triggerNew() override;

    /// The single filter state: the header search box and the page box both call this.
    void setSearchText(const QString& text);
    QString searchText() const;

    /// Index of the visible empty state, for tests.
    int currentViewIndex() const;

    /// Rows currently passing the search and chip filters.
    int visibleRowCount() const;

    /// Writes the visible rows to `path`. Extracted from the toolbar handler so tests
    /// never need a file dialog.
    bool exportTo(const QString& path, int* exportedRows, QString* error);

public slots:
    /// Opens the dialog in Create mode and saves a new application.
    void newApplication();

    /// Asks for confirmation, then deletes `id` with everything that cascades from it.
    void deleteApplication(int id);

    /// Moves `id` to `status`, which appends a timeline row (SPEC §3.3 A5).
    void changeStatus(int id, JobPrep::Domain::ApplicationStatus status);

protected:
    /// Enter and Delete are handled here so they only apply while the table has focus.
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onRowsChanged();
    void onContextMenuRequested(const QPoint& position);

private:
    void setupUi(QVBoxLayout* layout);
    void setupToolbar(QVBoxLayout* layout);
    void setupTable(QVBoxLayout* layout);
    void setupEmptyStates();
    void setupContextMenu();
    void refreshViewState();
    void refreshFooterCounts();
    void restoreSelection();
    std::optional<int> selectedId() const;
    void selectId(int id);
    void editApplication(int id);
    QList<JobPrep::Services::ExportRow> buildExportRows() const;

    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::InterviewRepository& m_interviews;
    JobPrep::Services::ExportService& m_exportService;

    Models::ApplicationTableModel* m_model{nullptr};
    Models::ApplicationFilterProxy* m_proxy{nullptr};
    QTableView* m_view{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    Widgets::ChipBar* m_chips{nullptr};
    QStackedWidget* m_stack{nullptr};
    QLabel* m_countsLabel{nullptr};
    QMenu* m_contextMenu{nullptr};

    QString m_searchText;
    /// Survives model resets, so a reload keeps the same row selected.
    std::optional<int> m_selectedId;
};

}  // namespace JobPrep::Ui::Pages