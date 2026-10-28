#include "UiTestHelpers.h"

#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/dialogs/SessionDialog.h"
#include "ui/dialogs/TrackDialog.h"

using namespace Qt::StringLiterals;

namespace JobPrep::UiTest {

namespace {

QMutex s_warningMutex;
QStringList s_capturedWarnings;
QtMessageHandler s_previousHandler = nullptr;

void testMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);
    if (type != QtWarningMsg && type != QtCriticalMsg && type != QtFatalMsg) return;
    if (isPlatformPluginNoise(msg)) return;

    QMutexLocker locker(&s_warningMutex);
    s_capturedWarnings.append(msg);
}

}  // namespace

bool isPlatformPluginNoise(const QString& msg) {
    return msg.contains(u"does not support propagateSizeHints"_s) ||
           msg.contains(u"does not support raise"_s) ||
           msg.contains(u"zwp_text_input_v3"_s) ||
           msg.contains(u"disableSurface"_s) ||
           msg.contains(u"systemTrayWindowChanged"_s);
}

void installWarningFilter() {
    clearCapturedWarnings();
    s_previousHandler = qInstallMessageHandler(testMessageHandler);
}

void uninstallWarningFilter() {
    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;
}

QStringList capturedWarnings() {
    QMutexLocker locker(&s_warningMutex);
    return s_capturedWarnings;
}

void clearCapturedWarnings() {
    QMutexLocker locker(&s_warningMutex);
    s_capturedWarnings.clear();
}

void isolateSettings(QTemporaryDir& tempDir) {
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.path());
    const QString uniqueAppName = u"JobPrepTest_%1"_s.arg(QCoreApplication::applicationPid());
    QCoreApplication::setOrganizationName(u"JobPrepTest"_s);
    QCoreApplication::setApplicationName(uniqueAppName);
}

QPushButton* buttonNamed(QWidget* root, const QString& objectName) {
    return childNamed<QPushButton>(root, objectName);
}

int countApplicationDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(widget)) ++count;
    }
    return count;
}

int countInterviewDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<JobPrep::Ui::Dialogs::InterviewDialog*>(widget)) ++count;
    }
    return count;
}

int countSessionDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<JobPrep::Ui::Dialogs::SessionDialog*>(widget)) ++count;
    }
    return count;
}

int countTrackDialogs() {
    int count = 0;
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget* widget : widgets) {
        if (qobject_cast<JobPrep::Ui::Dialogs::TrackDialog*>(widget)) ++count;
    }
    return count;
}

bool activateWindow(QWidget* window) {
    if (!window) return false;
    if (!window->isVisible()) {
        window->show();
    }
    window->activateWindow();
    window->raise();
    QCoreApplication::processEvents();
    if (!window->isActiveWindow()) {
        const bool active = QTest::qWaitForWindowActive(window, 1000);
        Q_UNUSED(active);
    }
    return true;
}

bool sendKeyClick(QWidget* window, Qt::Key key, Qt::KeyboardModifiers modifiers) {
    if (!activateWindow(window)) return false;
    QWidget* target = window->focusWidget();
    if (!target) target = window;
    QTest::keyClick(target, key, modifiers);
    QCoreApplication::processEvents();
    return true;
}

bool sendKeySequence(QWidget* window, const QKeySequence& sequence) {
    if (!activateWindow(window)) return false;
    if (sequence.isEmpty()) return false;

    for (int i = 0; i < sequence.count(); ++i) {
        const QKeyCombination comb = sequence[static_cast<uint>(i)];
        if (!sendKeyClick(window, comb.key(), comb.keyboardModifiers())) {
            return false;
        }
    }
    return true;
}

}  // namespace JobPrep::UiTest

#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/EnumStrings.h"
#include "domain/Structs.h"
#include "models/ApplicationTableModel.h"
#include "services/AgendaService.h"
#include "services/SettingsService.h"
#include "ui/MainWindow.h"
#include "ui/theme/ThemeManager.h"
#include "ui/pages/CalendarPage.h"
#include "ui/pages/JobsPage.h"
#include "ui/pages/StudyPage.h"
#include "ui/widgets/ChipBar.h"
#include "ui/widgets/MonthView.h"
#include "ui/widgets/TopicBoardColumn.h"
#include <QLineEdit>
#include <QListView>
#include <QTableView>

namespace JobPrep::UiTest {

using namespace JobPrep::Domain;
using JobPrep::Models::ApplicationTableModel;

WindowFixture::WindowFixture() {
    reset();
}

WindowFixture::~WindowFixture() = default;

void WindowFixture::reset() {
    m_window.reset();
    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
    QCoreApplication::processEvents();
}

JobsFixture::JobsFixture() = default;
JobsFixture::~JobsFixture() = default;

void JobsFixture::resetData() {
    m_page.reset();
    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);
    m_page = std::make_unique<JobPrep::Ui::Pages::JobsPage>(
        m_ctx->applications(), m_ctx->interviews(), m_ctx->exportService());
    m_page->show();
    QCoreApplication::processEvents();
}

QTableView* JobsFixture::table() const {
    return childNamed<QTableView>(m_page.get(), u"JobsTableView"_s);
}

void JobsFixture::addApplication(const QString& company, const QString& position,
                                ApplicationStatus status, const QString& notes) {
    JobApplication app;
    app.company = company;
    app.position = position;
    app.status = status;
    app.notes = notes;
    m_ctx->applications().insert(app);
}

QString JobsFixture::companyAt(int row) const {
    return table()->model()->index(row, ApplicationTableModel::CompanyColumn).data().toString();
}

int JobsFixture::idAt(int row) const {
    return table()->model()
        ->index(row, ApplicationTableModel::CompanyColumn)
        .data(ApplicationTableModel::IdRole)
        .toInt();
}

void JobsFixture::saveNextDialog(const QString& company, const QString& position) {
    withNextModal([company, position](QWidget* modal) {
        auto* dialog = qobject_cast<JobPrep::Ui::Dialogs::ApplicationDialog*>(modal);
        if (dialog) {
            childNamed<QLineEdit>(dialog, u"FieldCompany"_s)->setText(company);
            childNamed<QLineEdit>(dialog, u"FieldPosition"_s)->setText(position);
            buttonNamed(dialog, u"DialogSaveBtn"_s)->click();
        }
    });
}

void JobsFixture::clickChip(int index) {
    auto* chips = childNamed<JobPrep::Ui::Widgets::ChipBar>(m_page.get(), u"JobsStatusChips"_s);
    if (!chips) return;
    const auto buttons = chips->findChildren<QPushButton*>();
    if (index >= 0 && index < buttons.size()) {
        buttons.at(index)->click();
        QCoreApplication::processEvents();
    }
}

CalendarFixture::CalendarFixture() = default;
CalendarFixture::~CalendarFixture() = default;

void CalendarFixture::resetData(const QDate& today) {
    m_page.reset();
    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);

    m_ctx->settings().setCalendarLayerInterviews(true);
    m_ctx->settings().setCalendarLayerFollowUps(true);
    m_ctx->settings().setCalendarLayerTargets(true);

    m_page = std::make_unique<JobPrep::Ui::Pages::CalendarPage>(
        m_ctx->interviews(), m_ctx->applications(), m_ctx->topics(), m_ctx->tracks(),
        m_ctx->agendaService(), m_ctx->settings(), m_ctx->themeManager(), nullptr,
        [today]() { return today; });
    m_page->resize(1200, 720);
    m_page->show();
    QCoreApplication::processEvents();
}

JobPrep::Ui::Widgets::MonthView* CalendarFixture::monthView() const {
    return childNamed<JobPrep::Ui::Widgets::MonthView>(m_page.get(), u"CalendarMonthView"_s);
}

QListView* CalendarFixture::agendaList() const {
    return childNamed<QListView>(m_page.get(), u"CalendarAgendaList"_s);
}

int CalendarFixture::addApplicationWithInterview(const QString& company, const QDate& day, int hour) {
    JobApplication app;
    app.company = company;
    app.position = u"Developer"_s;
    if (!m_ctx->applications().insert(app)) {
        return 0;
    }

    Interview interview;
    interview.applicationId = app.id;
    interview.startAt = QDateTime(day, QTime(hour, 0));
    m_ctx->interviews().insert(interview);
    return app.id;
}

int CalendarFixture::addStudyTarget(const QDate& day) {
    Track track;
    track.name = u"English"_s;
    if (!m_ctx->tracks().insert(track)) {
        return 0;
    }

    Topic topic;
    topic.trackId = track.id;
    topic.title = u"Review STL containers"_s;
    topic.targetDate = day;
    m_ctx->topics().insert(topic);
    return track.id;
}

StudyFixture::StudyFixture() = default;
StudyFixture::~StudyFixture() = default;

void StudyFixture::resetData(const QDate& today) {
    m_page.reset();
    m_ctx = std::make_unique<JobPrep::App::AppContext>(u":memory:"_s);

    m_ctx->settings().setStudyTrackFilter(-1);

    m_page = std::make_unique<JobPrep::Ui::Pages::StudyPage>(
        m_ctx->topics(), m_ctx->tracks(), m_ctx->sessions(), m_ctx->settings(),
        m_ctx->themeManager(), nullptr, [today]() { return today; });
    m_page->resize(1200, 720);
    m_page->show();
    QCoreApplication::processEvents();
}

JobPrep::Ui::Widgets::TopicBoardColumn* StudyFixture::column(JobPrep::Domain::TopicStatus status) const {
    const QString name = [&]() {
        switch (status) {
            case JobPrep::Domain::TopicStatus::Backlog: return u"StudyColumnBacklog"_s;
            case JobPrep::Domain::TopicStatus::InProgress: return u"StudyColumnInProgress"_s;
            case JobPrep::Domain::TopicStatus::Review: return u"StudyColumnReview"_s;
            case JobPrep::Domain::TopicStatus::Done: return u"StudyColumnDone"_s;
        }
        return u""_s;
    }();
    return childNamed<JobPrep::Ui::Widgets::TopicBoardColumn>(m_page.get(), name);
}


}  // namespace JobPrep::UiTest
