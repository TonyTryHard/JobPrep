#pragma once

#include <functional>
#include <QDate>
#include <QList>
#include "domain/Structs.h"
#include "ui/PageBase.h"

class QLabel;
class QListView;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Models {
class AgendaListModel;
}

namespace JobPrep::Services {
class AgendaService;
class SettingsService;
}  // namespace JobPrep::Services

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Widgets {
class MonthView;
}

namespace JobPrep::Ui::Pages {

/// Calendar and agenda page (SPEC §3.4 C6): a month grid with per-kind markers on the
/// left and a day agenda panel with add/entry points on the right, split by a persistent
/// `QSplitter`. Appointments are owned by other pages (Jobs, Study); this page only reads
/// them through `AgendaService`.
class CalendarPage : public PageBase {
    Q_OBJECT

public:
    CalendarPage(JobPrep::Data::InterviewRepository& interviews,
                 JobPrep::Data::ApplicationRepository& apps,
                 JobPrep::Data::TopicRepository& topics,
                 JobPrep::Data::TrackRepository& tracks,
                 JobPrep::Services::AgendaService& agenda,
                 JobPrep::Services::SettingsService& settings,
                 JobPrep::Ui::Theme::ThemeManager& theme,
                 QWidget* parent = nullptr,
                 std::function<QDate()> todayProvider = {});
    ~CalendarPage() override = default;

    /// The day currently selected in the month grid.
    QDate selectedDay() const;

    /// True when `date` has at least one agenda item under the active layer settings.
    bool hasItemsOn(QDate date) const;

    /// Selects the interview's date in MonthView and opens the interview editor. Returns true if found.
    bool openInterview(int id);

    void onActivated() override;
    void triggerNew() override;

signals:
    /// Emitted when clicking or activating a study target agenda row (B9).
    void studyTopicActivated(int topicId);

public slots:
    /// Opens a new-interview dialog. An invalid `date` falls back to the selected day.
    void newInterview(QDate date = QDate());

private slots:
    void addFollowUp();
    void switchMonth(int delta);
    void reloadMarkers();
    void reloadAgenda();
    void openItem(const QModelIndex& index);

private:
    void setupUi();
    void setupToolbar(QVBoxLayout* layout);
    void setupSplitter(QVBoxLayout* layout);
    void setupAgendaPanel(QWidget* panel);
    QList<JobPrep::Domain::AgendaItem> agendaItems(QDate from, QDate to) const;
    void refreshMarkersAndAgenda();
    void updateMonthTitle();
    void saveSplitterState();
    void syncLayerChips();

    JobPrep::Data::InterviewRepository& m_interviews;
    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::TopicRepository& m_topics;
    JobPrep::Data::TrackRepository& m_tracks;
    JobPrep::Services::AgendaService& m_agenda;
    JobPrep::Services::SettingsService& m_settings;
    JobPrep::Ui::Theme::ThemeManager& m_theme;
    std::function<QDate()> m_todayProvider;

    Widgets::MonthView* m_monthView{nullptr};
    QLabel* m_monthTitleLabel{nullptr};
    QToolButton* m_prevBtn{nullptr};
    QToolButton* m_nextBtn{nullptr};
    QPushButton* m_todayBtn{nullptr};
    QPushButton* m_interviewsChip{nullptr};
    QPushButton* m_followUpsChip{nullptr};
    QPushButton* m_targetsChip{nullptr};
    QSplitter* m_splitter{nullptr};
    QStackedWidget* m_agendaStack{nullptr};
    QLabel* m_agendaTitleLabel{nullptr};
    QListView* m_agendaView{nullptr};
    Models::AgendaListModel* m_agendaModel{nullptr};
};

}  // namespace JobPrep::Ui::Pages