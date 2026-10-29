#pragma once

#include <QApplication>
#include <QKeySequence>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QWidget>

namespace JobPrep::UiTest {

/// Installs a Qt message handler that captures warnings/criticals, ignoring known
/// offscreen platform plugin noise.
void installWarningFilter();
void uninstallWarningFilter();
QStringList capturedWarnings();
void clearCapturedWarnings();
bool isPlatformPluginNoise(const QString& msg);

/// Checks if any unexpected warnings were captured. Fails with QFAIL if not empty.
#define JOBPREP_VERIFY_ZERO_WARNINGS() \
    do { \
        const QStringList _w = JobPrep::UiTest::capturedWarnings(); \
        if (!_w.isEmpty()) { \
            QFAIL(qPrintable(QString(u"Captured unexpected Qt warnings/criticals:\n%1"_s).arg(_w.join(u"\n"_s)))); \
        } \
    } while (0)

/// Configures QSettings to write into an isolated temporary directory in IniFormat
/// so tests never collide on disk when executed in parallel (ctest -j).
void isolateSettings(QTemporaryDir& tempDir);

/// Runs `action` against the next modal widget once `exec()`'s event loop spins.
template <typename Action>
void withNextModal(Action action) {
    QTimer::singleShot(0, [action]() {
        if (auto* modal = qobject_cast<QWidget*>(QApplication::activeModalWidget())) {
            action(modal);
        }
    });
}

/// Helper that inspects the next modal widget as a QMessageBox.
template <typename Action>
void answerNextMessageBox(Action action) {
    withNextModal([action](QWidget* modal) {
        if (auto* box = qobject_cast<QMessageBox*>(modal)) {
            action(box);
        }
    });
}

/// Answers the next modal widget, polling every 5ms until one appears.
template <typename Action>
void answerModalWhenReady(Action action) {
    auto* timer = new QTimer;
    timer->setInterval(5);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, action]() {
        if (auto* modal = qobject_cast<QWidget*>(QApplication::activeModalWidget())) {
            timer->stop();
            timer->deleteLater();
            action(modal);
            return;
        }
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (w->isModal() && w->isVisible()) {
                timer->stop();
                timer->deleteLater();
                action(w);
                return;
            }
        }
    });
    timer->start();
}

/// Finds a child widget by name.
template <typename T>
T* childNamed(QWidget* root, const QString& objectName) {
    return root ? root->findChild<T*>(objectName) : nullptr;
}

QPushButton* buttonNamed(QWidget* root, const QString& objectName);

/// Counts top-level dialogs of specific types across QApplication.
int countApplicationDialogs();
int countInterviewDialogs();
int countSessionDialogs();
int countTrackDialogs();

/// Ensures `window` is shown, raised, and active. Returns true on success.
bool activateWindow(QWidget* window);

/// Dispatches a real key event to the focused widget of a shown and active window.
/// If no child has focus, dispatches to `window`.
bool sendKeyClick(QWidget* window, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

/// Decomposes `sequence` into key combinations and dispatches real key clicks to `window`.
bool sendKeySequence(QWidget* window, const QKeySequence& sequence);

}  // namespace JobPrep::UiTest

namespace JobPrep::App {
class AppContext;
}

namespace JobPrep::Ui {
class MainWindow;
namespace Pages {
class JobsPage;
class CalendarPage;
class StudyPage;
}
namespace Widgets {
class MonthView;
class TopicBoardColumn;
class ChipBar;
}
}

class QTableView;
class QListView;

#include <domain/Enums.h>

namespace JobPrep::UiTest {

class WindowFixture {
public:
    WindowFixture();
    ~WindowFixture();

    void reset();
    JobPrep::App::AppContext& context() { return *m_ctx; }
    JobPrep::Ui::MainWindow& window() { return *m_window; }

private:
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
};

class JobsFixture {
public:
    JobsFixture();
    ~JobsFixture();

    void resetData();
    JobPrep::App::AppContext& context() { return *m_ctx; }
    JobPrep::Ui::Pages::JobsPage* page() { return m_page.get(); }
    QTableView* table() const;
    void addApplication(const QString& company, const QString& position,
                        JobPrep::Domain::ApplicationStatus status = JobPrep::Domain::ApplicationStatus::Applied,
                        const QString& notes = QString());
    QString companyAt(int row) const;
    int idAt(int row) const;
    void saveNextDialog(const QString& company, const QString& position);
    void clickChip(int index);

private:
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::Pages::JobsPage> m_page;
};

class CalendarFixture {
public:
    CalendarFixture();
    ~CalendarFixture();

    void resetData(const QDate& today);
    JobPrep::App::AppContext& context() { return *m_ctx; }
    JobPrep::Ui::Pages::CalendarPage* page() { return m_page.get(); }
    JobPrep::Ui::Widgets::MonthView* monthView() const;
    QListView* agendaList() const;
    int addApplicationWithInterview(const QString& company, const QDate& day, int hour);
    int addStudyTarget(const QDate& day);

private:
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::Pages::CalendarPage> m_page;
};

class StudyFixture {
public:
    StudyFixture();
    ~StudyFixture();

    void resetData(const QDate& today = QDate(2026, 10, 19));
    JobPrep::App::AppContext& context() { return *m_ctx; }
    JobPrep::Ui::Pages::StudyPage* page() { return m_page.get(); }
    JobPrep::Ui::Widgets::TopicBoardColumn* column(JobPrep::Domain::TopicStatus status) const;

private:
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::Pages::StudyPage> m_page;
};

}  // namespace JobPrep::UiTest
