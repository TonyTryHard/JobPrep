#include "ui/pages/CalendarPage.h"

#include <algorithm>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpacerItem>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>
#include "data/ApplicationRepository.h"
#include "data/DbFormat.h"
#include "data/InterviewRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "models/AgendaListModel.h"
#include "services/AgendaService.h"
#include "services/SettingsService.h"
#include "ui/delegates/AgendaItemDelegate.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/dialogs/FollowUpDialog.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/theme/AgendaKindStyle.h"
#include "ui/theme/IconProvider.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/MonthView.h"

using namespace Qt::StringLiterals;
using JobPrep::Domain::AgendaKind;

namespace JobPrep::Ui::Pages {

namespace {

void setLayerChipChecked(QPushButton* button, bool checked) {
    if (button) button->setChecked(checked);
}

/// A filled circle in the kind color, used as the layer chip's dot (Amendment C8).
QIcon kindDotIcon(const QColor& color) {
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

QPushButton* makeLayerChip(const QString& text, JobPrep::Domain::AgendaKind kind,
                           QWidget* parent) {
    auto* chip = new QPushButton(text, parent);
    chip->setCheckable(true);
    chip->setCursor(Qt::PointingHandCursor);
    chip->setProperty("calendarLayer", true);
    const QColor color = JobPrep::Ui::Theme::AgendaKindStyle::color(kind);
    chip->setIcon(kindDotIcon(color));
    chip->setToolTip(JobPrep::Ui::Theme::AgendaKindStyle::label(kind) +
                     (u" \u2013 %1"_s).arg(QObject::tr("show or hide this layer")));
    return chip;
}

}  // namespace

CalendarPage::CalendarPage(JobPrep::Data::InterviewRepository& interviews,
                           JobPrep::Data::ApplicationRepository& apps,
                           JobPrep::Data::TopicRepository& topics,
                           JobPrep::Data::TrackRepository& tracks,
                           JobPrep::Services::AgendaService& agenda,
                           JobPrep::Services::SettingsService& settings,
                           JobPrep::Ui::Theme::ThemeManager& theme,
                           QWidget* parent,
                           std::function<QDate()> todayProvider)
    : PageBase(tr("Calendar"), parent),
      m_interviews(interviews),
      m_apps(apps),
      m_topics(topics),
      m_tracks(tracks),
      m_agenda(agenda),
      m_settings(settings),
      m_theme(theme),
      m_todayProvider(todayProvider ? std::move(todayProvider)
                                    : std::function<QDate()>([] { return QDate::currentDate(); })) {
    setupUi();

    connect(&m_interviews, &JobPrep::Data::InterviewRepository::changed, this,
            &CalendarPage::refreshMarkersAndAgenda);
    connect(&m_apps, &JobPrep::Data::ApplicationRepository::changed, this,
            &CalendarPage::refreshMarkersAndAgenda);
    connect(&m_topics, &JobPrep::Data::TopicRepository::changed, this,
            &CalendarPage::refreshMarkersAndAgenda);
    connect(&m_tracks, &JobPrep::Data::TrackRepository::changed, this,
            &CalendarPage::refreshMarkersAndAgenda);

    connect(&m_settings, &JobPrep::Services::SettingsService::firstDayOfWeekChanged, this,
            [this](int day) {
                m_monthView->setFirstDayOfWeek(day);
                refreshMarkersAndAgenda();
            });
    connect(&m_settings, &JobPrep::Services::SettingsService::calendarLayerInterviewsChanged,
            this, [this](bool) { refreshMarkersAndAgenda(); });
    connect(&m_settings, &JobPrep::Services::SettingsService::calendarLayerFollowUpsChanged,
            this, [this](bool) { refreshMarkersAndAgenda(); });
    connect(&m_settings, &JobPrep::Services::SettingsService::calendarLayerTargetsChanged, this,
            [this](bool) { refreshMarkersAndAgenda(); });

    syncLayerChips();
    updateMonthTitle();
    refreshMarkersAndAgenda();
}

void CalendarPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(16);

    setupToolbar(layout);
    setupSplitter(layout);
}

void CalendarPage::setupToolbar(QVBoxLayout* layout) {
    auto* bar = new QWidget(this);
    bar->setObjectName(u"CalendarToolbar"_s);
    auto* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(8);

    m_prevBtn = new QToolButton(bar);
    m_prevBtn->setObjectName(u"CalendarPrevBtn"_s);
    m_prevBtn->setIcon(JobPrep::Ui::Theme::IconProvider::icon(u"chevron-left"_s,
                                                              m_theme.currentTokens().textMuted));
    m_prevBtn->setToolTip(tr("Previous month (Page Up)"));
    m_prevBtn->setCursor(Qt::PointingHandCursor);
    m_prevBtn->setText(tr("Previous month"));
    m_prevBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_nextBtn = new QToolButton(bar);
    m_nextBtn->setObjectName(u"CalendarNextBtn"_s);
    m_nextBtn->setIcon(JobPrep::Ui::Theme::IconProvider::icon(u"chevron-right"_s,
                                                              m_theme.currentTokens().textMuted));
    m_nextBtn->setToolTip(tr("Next month (Page Down)"));
    m_nextBtn->setCursor(Qt::PointingHandCursor);
    m_nextBtn->setText(tr("Next month"));
    m_nextBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_monthTitleLabel = new QLabel(bar);
    m_monthTitleLabel->setObjectName(u"CalendarMonthTitle"_s);
    m_monthTitleLabel->setTextFormat(Qt::PlainText);

    m_todayBtn = new QPushButton(tr("Today"), bar);
    m_todayBtn->setObjectName(u"CalendarTodayBtn"_s);
    m_todayBtn->setToolTip(tr("Go to today"));
    m_todayBtn->setCursor(Qt::PointingHandCursor);

    barLayout->addWidget(m_prevBtn);
    barLayout->addWidget(m_nextBtn);
    barLayout->addSpacing(8);
    barLayout->addWidget(m_monthTitleLabel, 1);
    barLayout->addWidget(m_todayBtn);

    m_interviewsChip = makeLayerChip(tr("Interviews"), AgendaKind::Interview, bar);
    m_interviewsChip->setObjectName(u"CalendarLayerInterviewBtn"_s);
    m_followUpsChip = makeLayerChip(tr("Follow-ups"), AgendaKind::FollowUp, bar);
    m_followUpsChip->setObjectName(u"CalendarLayerFollowUpBtn"_s);
    m_targetsChip = makeLayerChip(tr("Study targets"), AgendaKind::StudyTarget, bar);
    m_targetsChip->setObjectName(u"CalendarLayerTargetBtn"_s);

    barLayout->addSpacing(16);
    barLayout->addWidget(m_interviewsChip);
    barLayout->addWidget(m_followUpsChip);
    barLayout->addWidget(m_targetsChip);

    layout->addWidget(bar);

    connect(m_prevBtn, &QToolButton::clicked, this, [this]() { switchMonth(-1); });
    connect(m_nextBtn, &QToolButton::clicked, this, [this]() { switchMonth(1); });
    connect(m_todayBtn, &QPushButton::clicked, this,
            [this]() { m_monthView->setSelectedDay(m_monthView->today()); });

    const auto bindLayerChip = [this](QPushButton* chip, void (JobPrep::Services::SettingsService::*setter)(bool)) {
        connect(chip, &QPushButton::toggled, this, [this, setter](bool checked) {
            (m_settings.*setter)(checked);
            refreshMarkersAndAgenda();
        });
    };
    bindLayerChip(m_interviewsChip, &JobPrep::Services::SettingsService::setCalendarLayerInterviews);
    bindLayerChip(m_followUpsChip, &JobPrep::Services::SettingsService::setCalendarLayerFollowUps);
    bindLayerChip(m_targetsChip, &JobPrep::Services::SettingsService::setCalendarLayerTargets);
}

void CalendarPage::setupSplitter(QVBoxLayout* layout) {
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName(u"CalendarSplitter"_s);
    m_splitter->setChildrenCollapsible(false);

    m_monthView = new Widgets::MonthView(m_theme, m_todayProvider, m_splitter);
    m_monthView->setObjectName(u"CalendarMonthView"_s);
    m_monthView->setFirstDayOfWeek(m_settings.firstDayOfWeek());
    m_splitter->addWidget(m_monthView);

    auto* panel = new QWidget(m_splitter);
    panel->setObjectName(u"CalendarAgendaPanel"_s);
    setupAgendaPanel(panel);
    m_splitter->addWidget(panel);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);

    const QByteArray splitterState = m_settings.calendarSplitterState();
    if (splitterState.isEmpty()) {
        m_splitter->setSizes({640, 320});
    } else {
        m_splitter->restoreState(splitterState);
    }

    layout->addWidget(m_splitter, 1);

    connect(m_splitter, &QSplitter::splitterMoved, this, &CalendarPage::saveSplitterState);

    connect(m_monthView, &Widgets::MonthView::monthChanged, this,
            [this](QDate) {
                updateMonthTitle();
                reloadMarkers();
            });
    connect(m_monthView, &Widgets::MonthView::selectionChanged, this,
            [this](QDate) { reloadAgenda(); });
    connect(m_monthView, &Widgets::MonthView::dayActivated, this,
            [this](QDate day) { newInterview(day); });
}

void CalendarPage::setupAgendaPanel(QWidget* panel) {
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(16, 16, 16, 16);
    panelLayout->setSpacing(12);

    auto* panelHeader = new QHBoxLayout;
    m_agendaTitleLabel = new QLabel(panel);
    m_agendaTitleLabel->setObjectName(u"CalendarAgendaTitle"_s);
    panelHeader->addWidget(m_agendaTitleLabel, 1);

    auto* addInterviewBtn = new QPushButton(tr("+ Interview"), panel);
    addInterviewBtn->setObjectName(u"CalendarAddInterviewBtn"_s);
    addInterviewBtn->setProperty("primary", true);
    addInterviewBtn->setToolTip(tr("Schedule an interview on the selected day (double-click a day)"));
    addInterviewBtn->setCursor(Qt::PointingHandCursor);

    auto* addFollowUpBtn = new QPushButton(tr("+ Follow-up"), panel);
    addFollowUpBtn->setObjectName(u"CalendarAddFollowUpBtn"_s);
    addFollowUpBtn->setToolTip(tr("Plan a follow-up action on the selected day"));
    addFollowUpBtn->setCursor(Qt::PointingHandCursor);

    auto* addTargetBtn = new QPushButton(tr("+ Study target"), panel);
    addTargetBtn->setObjectName(u"CalendarAddTargetBtn"_s);
    addTargetBtn->setEnabled(false);
    addTargetBtn->setToolTip(tr("Study targets open in M4"));

    panelHeader->addWidget(addInterviewBtn);
    panelHeader->addWidget(addFollowUpBtn);
    panelHeader->addWidget(addTargetBtn);
    panelLayout->addLayout(panelHeader);

    m_agendaStack = new QStackedWidget(panel);
    m_agendaStack->setObjectName(u"CalendarAgendaStack"_s);

    m_agendaView = new QListView(panel);
    m_agendaView->setObjectName(u"CalendarAgendaList"_s);
    m_agendaView->setUniformItemSizes(true);
    m_agendaView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_agendaView->setMouseTracking(true);
    m_agendaView->setFrameShape(QFrame::NoFrame);
    m_agendaModel = new Models::AgendaListModel(m_agendaView);
    m_agendaView->setModel(m_agendaModel);
    m_agendaView->setItemDelegate(
        new JobPrep::Ui::Delegates::AgendaItemDelegate(m_theme, m_agendaView));
    m_agendaStack->addWidget(m_agendaView);

    auto* emptyState = new Widgets::EmptyState(
        u"calendar"_s, tr("Nothing Planned"),
        tr("Use + Interview or + Follow-up to schedule something here."),
        tr("+ Interview"), m_agendaStack);
    m_agendaStack->addWidget(emptyState);

    panelLayout->addWidget(m_agendaStack, 1);

    connect(addInterviewBtn, &QPushButton::clicked, this, [this]() { newInterview(); });
    connect(addFollowUpBtn, &QPushButton::clicked, this, &CalendarPage::addFollowUp);
    connect(emptyState, &Widgets::EmptyState::actionClicked, this, [this]() { newInterview(); });
    connect(m_agendaView, &QListView::clicked, this, &CalendarPage::openItem);
    connect(m_agendaView, &QListView::activated, this, &CalendarPage::openItem);
}

QDate CalendarPage::selectedDay() const {
    return m_monthView ? m_monthView->selectedDay() : QDate();
}

void CalendarPage::onActivated() {
    refreshMarkersAndAgenda();
}

bool CalendarPage::hasItemsOn(QDate date) const {
    if (!date.isValid()) return false;
    return !agendaItems(date, date).isEmpty();
}

QList<JobPrep::Domain::AgendaItem> CalendarPage::agendaItems(QDate from, QDate to) const {
    auto items = m_agenda.items(from, to);
    const auto keepKind = [](const QList<JobPrep::Domain::AgendaItem>& list, AgendaKind kind,
                             bool enabled) {
        if (enabled) return list;
        QList<JobPrep::Domain::AgendaItem> filtered;
        filtered.reserve(list.size());
        for (const auto& item : list) {
            if (item.kind != kind) filtered.append(item);
        }
        return filtered;
    };
    items = keepKind(items, AgendaKind::Interview, m_settings.calendarLayerInterviews());
    items = keepKind(items, AgendaKind::FollowUp, m_settings.calendarLayerFollowUps());
    items = keepKind(items, AgendaKind::StudyTarget, m_settings.calendarLayerTargets());
    return items;
}

void CalendarPage::reloadMarkers() {
    if (!m_monthView) return;
    const auto range = m_monthView->visibleRange();
    QHash<QDate, Widgets::MonthView::DayMarkers> markers;
    const auto items = agendaItems(range.first, range.second);
    for (const auto& item : items) {
        if (!item.date.isValid()) continue;
        auto& marker = markers[item.date];
        marker.total += 1;
        if (marker.kinds.size() < 3 && !marker.kinds.contains(item.kind)) {
            marker.kinds.append(item.kind);
        }
    }
    m_monthView->setMarkers(markers);
}

void CalendarPage::reloadAgenda() {
    if (!m_agendaModel || !m_monthView) return;
    const QDate day = selectedDay();
    m_agendaModel->setItems(agendaItems(day, day));

    const QDate today = m_todayProvider();
    QString title = u"Agenda \u00b7 %1"_s.arg(JobPrep::Data::DbFormat::toText(day));
    if (day == today) title.append(u" (Today)"_s);
    m_agendaTitleLabel->setText(title);

    const bool empty = m_agendaModel->rowCount() == 0;
    m_agendaStack->setCurrentIndex(empty ? 1 : 0);
}

void CalendarPage::refreshMarkersAndAgenda() {
    reloadMarkers();
    reloadAgenda();
}

void CalendarPage::updateMonthTitle() {
    if (m_monthTitleLabel && m_monthView) {
        m_monthTitleLabel->setText(m_monthView->monthTitle());
    }
}

void CalendarPage::saveSplitterState() {
    if (m_splitter) m_settings.setCalendarSplitterState(m_splitter->saveState());
}

void CalendarPage::syncLayerChips() {
    const QSignalBlocker blocker(m_interviewsChip);
    setLayerChipChecked(m_interviewsChip, m_settings.calendarLayerInterviews());
    setLayerChipChecked(m_followUpsChip, m_settings.calendarLayerFollowUps());
    setLayerChipChecked(m_targetsChip, m_settings.calendarLayerTargets());
}

void CalendarPage::switchMonth(int delta) {
    if (!m_monthView) return;
    m_monthView->setVisibleMonth(m_monthView->visibleMonth().addMonths(delta));
    m_monthView->setFocus(Qt::OtherFocusReason);
}

void CalendarPage::newInterview(QDate date) {
    const QDate day = date.isValid() ? date : selectedDay();
    JobPrep::Ui::Dialogs::InterviewDialog dialog(m_apps, m_interviews,
                                                 JobPrep::Ui::Dialogs::InterviewDialog::Create, this);
    dialog.setStartDate(day);
    dialog.exec();
}

void CalendarPage::addFollowUp() {
    JobPrep::Ui::Dialogs::FollowUpDialog dialog(m_apps, selectedDay(), this);
    dialog.exec();
}

void CalendarPage::openItem(const QModelIndex& index) {
    if (!index.isValid()) return;
    const auto kind = static_cast<AgendaKind>(
        index.data(JobPrep::Models::AgendaListModel::KindRole).toInt());
    const int refId = index.data(JobPrep::Models::AgendaListModel::IdRole).toInt();
    switch (kind) {
        case AgendaKind::Interview: {
            const auto stored = m_interviews.byId(refId);
            if (!stored) return;
            JobPrep::Ui::Dialogs::InterviewDialog dialog(
                m_apps, m_interviews, JobPrep::Ui::Dialogs::InterviewDialog::Edit, this);
            dialog.setInterview(*stored);
            dialog.exec();
            return;
        }
        case AgendaKind::FollowUp: {
            const auto stored = m_apps.byId(refId);
            if (!stored) return;
            JobPrep::Ui::Dialogs::ApplicationDialog dialog(
                m_apps, m_interviews, JobPrep::Ui::Dialogs::ApplicationDialog::Edit, this);
            dialog.setApplication(*stored);
            dialog.exec();
            return;
        }
        case AgendaKind::StudyTarget: {
            emit studyTopicActivated(refId);
            return;
        }
    }
}

void CalendarPage::triggerNew() {
    newInterview();
}

}  // namespace JobPrep::Ui::Pages