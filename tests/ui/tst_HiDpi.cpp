#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QElapsedTimer> // CI-DIAG
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QScreen>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QVBoxLayout>
#include <atomic> // CI-DIAG
#include <chrono> // CI-DIAG
#include <mutex> // CI-DIAG
#include <string> // CI-DIAG
#include <thread> // CI-DIAG

#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "services/SeedService.h"
#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/dialogs/ApplicationDialog.h"
#include "ui/dialogs/FollowUpDialog.h"
#include "ui/dialogs/InterviewDialog.h"
#include "ui/dialogs/SessionDialog.h"
#include "ui/dialogs/TrackDialog.h"
#include "ui/theme/ThemeManager.h"
#include "UiTestHelpers.h"

using namespace Qt::StringLiterals;
using JobPrep::Ui::Theme::ThemeMode;

// CI-DIAG: Watchdog and progress instrumentation for CI hang diagnostics
static std::mutex s_diagMutex; // CI-DIAG
static std::string s_lastProgress = "uninitialized"; // CI-DIAG
static std::chrono::steady_clock::time_point s_lastProgressTime; // CI-DIAG
static std::atomic<bool> s_watchdogStop{false}; // CI-DIAG
static std::thread s_watchdogThread; // CI-DIAG
static QElapsedTimer s_diagTimer; // CI-DIAG
// CI-DIAG
static void updateProgress(const QString& msg) { // CI-DIAG
    const qint64 elapsedMs = s_diagTimer.isValid() ? s_diagTimer.elapsed() : 0; // CI-DIAG
    const QString fullMsg = QString(u"CI-DIAG progress: %1 t=%2ms"_s).arg(msg).arg(elapsedMs); // CI-DIAG
    { // CI-DIAG
        std::lock_guard<std::mutex> lock(s_diagMutex); // CI-DIAG
        s_lastProgress = fullMsg.toStdString(); // CI-DIAG
        s_lastProgressTime = std::chrono::steady_clock::now(); // CI-DIAG
    } // CI-DIAG
    qInfo("%s", qPrintable(fullMsg)); // CI-DIAG
    fflush(stderr); // CI-DIAG
} // CI-DIAG
// CI-DIAG
static void startWatchdog() { // CI-DIAG
    s_diagTimer.start(); // CI-DIAG
    { // CI-DIAG
        std::lock_guard<std::mutex> lock(s_diagMutex); // CI-DIAG
        s_lastProgress = "watchdog started"; // CI-DIAG
        s_lastProgressTime = std::chrono::steady_clock::now(); // CI-DIAG
    } // CI-DIAG
    s_watchdogStop.store(false); // CI-DIAG
    s_watchdogThread = std::thread([]() { // CI-DIAG
        while (!s_watchdogStop.load()) { // CI-DIAG
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // CI-DIAG
            if (s_watchdogStop.load()) break; // CI-DIAG
            std::string lastProg; // CI-DIAG
            std::chrono::steady_clock::time_point lastTime; // CI-DIAG
            { // CI-DIAG
                std::lock_guard<std::mutex> lock(s_diagMutex); // CI-DIAG
                lastProg = s_lastProgress; // CI-DIAG
                lastTime = s_lastProgressTime; // CI-DIAG
            } // CI-DIAG
            const auto sec = std::chrono::duration_cast<std::chrono::seconds>( // CI-DIAG
                std::chrono::steady_clock::now() - lastTime).count(); // CI-DIAG
            if (sec >= 240) { // CI-DIAG
                fprintf(stderr, "\n=== CI-DIAG WATCHDOG TIMEOUT: 240s without progress ===\n"); // CI-DIAG
                fprintf(stderr, "Last progress: %s\n", lastProg.c_str()); // CI-DIAG
                fprintf(stderr, "Watchdog Thread ID: %llu\n", static_cast<unsigned long long>( // CI-DIAG
                    std::hash<std::thread::id>{}(std::this_thread::get_id()))); // CI-DIAG
                fflush(stderr); // CI-DIAG
                std::_Exit(3); // CI-DIAG
            } // CI-DIAG
        } // CI-DIAG
    }); // CI-DIAG
} // CI-DIAG
// CI-DIAG
static void stopWatchdog() { // CI-DIAG
    s_watchdogStop.store(true); // CI-DIAG
    if (s_watchdogThread.joinable()) { // CI-DIAG
        s_watchdogThread.join(); // CI-DIAG
    } // CI-DIAG
} // CI-DIAG
struct ClippingViolation {
    QString widgetClass;
    QString objectName;
    QString text;
    QString context;
    QString reason;
};

class tst_HiDpi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDevicePixelRatio();
    void testPagesHiDpi();
    void testDialogsHiDpi();
    void testDialogsFitSmallScreen();
    void testScreenshotsHiDpi();

private:
    QList<ClippingViolation> checkClipping(QWidget* root, const QString& contextDescription);
    std::unique_ptr<JobPrep::UiTest::WindowFixture> m_fix;
    QTemporaryDir m_settingsDir;
};

void tst_HiDpi::initTestCase() {
    startWatchdog(); // CI-DIAG
    updateProgress(u"initTestCase start"_s); // CI-DIAG
    JobPrep::UiTest::applyTestFontFamily();
    JobPrep::UiTest::installWarningFilter();
    JobPrep::UiTest::isolateSettings(m_settingsDir);
    m_fix = std::make_unique<JobPrep::UiTest::WindowFixture>();

    auto& ctx = m_fix->context();
    ctx.seedService().loadSampleTopics();

    // Add a sample application with interview and follow-up
    JobPrep::Domain::JobApplication app;
    app.company = u"Acme Corp"_s;
    app.position = u"Senior C++ Developer"_s;
    app.status = JobPrep::Domain::ApplicationStatus::Technical;
    app.appliedDate = QDate(2026, 10, 1);
    app.nextAction = u"Technical interview"_s;
    app.nextActionDate = QDate(2026, 11, 5);
    app.contactName = u"Jane Doe"_s;
    app.contactEmail = u"jane@acme.example"_s;
    app.notes = u"Prepared modern C++ topics"_s;
    ctx.applications().insert(app);

    JobPrep::Domain::Interview interview;
    interview.applicationId = app.id;
    interview.startAt = QDateTime(QDate(2026, 11, 5), QTime(14, 0));
    interview.type = JobPrep::Domain::InterviewType::Technical;
    interview.durationMin = 60;
    interview.notes = u"Focus on concurrency and memory model"_s;
    ctx.interviews().insert(interview);
    updateProgress(u"initTestCase end"_s); // CI-DIAG
}

void tst_HiDpi::cleanupTestCase() {
    updateProgress(u"cleanupTestCase start"_s); // CI-DIAG
    m_fix.reset();
    JobPrep::UiTest::uninstallWarningFilter();
    updateProgress(u"cleanupTestCase end"_s); // CI-DIAG
    stopWatchdog(); // CI-DIAG
}

void tst_HiDpi::testDevicePixelRatio() {
    updateProgress(u"testDevicePixelRatio start"_s); // CI-DIAG
    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const qreal dpr = screen->devicePixelRatio();
    fprintf(stdout, "DPI_RATIO: %f\n", static_cast<double>(dpr));
    fflush(stdout);
    // When QT_SCALE_FACTOR=1.5 is set, screen->devicePixelRatio() is 1.5
    // Allow small epsilon for floating point comparison
    QVERIFY(qAbs(dpr - 1.5) < 0.05 || qAbs(dpr - 1.0) < 0.05 || qAbs(dpr - 2.0) < 0.05);
    updateProgress(u"testDevicePixelRatio end"_s); // CI-DIAG
}

static QString stripMnemonic(const QString& raw) {
    QString res;
    res.reserve(raw.size());
    for (int i = 0; i < raw.size(); ++i) {
        if (raw[i] == u'&') {
            if (i + 1 < raw.size() && raw[i + 1] == u'&') {
                res.append(u'&');
                ++i;
            }
        } else {
            res.append(raw[i]);
        }
    }
    return res;
}

QList<ClippingViolation> tst_HiDpi::checkClipping(QWidget* root, const QString& contextDescription) {
    QList<ClippingViolation> violations;
    if (!root) {
        return violations;
    }

    const auto children = root->findChildren<QWidget*>();
    for (QWidget* w : children) {
        if (!w->isVisible() || w->width() <= 0 || w->height() <= 0) {
            continue;
        }

        // Skip Qt internal sub-controls and scroll corners
        const QString className = QString::fromUtf8(w->metaObject()->className());
        if (w->objectName().startsWith(u"qt_"_s) ||
            className == u"QLineEditIconButton"_s ||
            className == u"QScrollAreaCorner"_s) {
            continue;
        }

        auto* label = qobject_cast<QLabel*>(w);
        auto* btn = qobject_cast<QAbstractButton*>(w);
        auto* grp = qobject_cast<QGroupBox*>(w);
        auto* combo = qobject_cast<QComboBox*>(w);
        auto* lineEdit = qobject_cast<QLineEdit*>(w);
        auto* spin = qobject_cast<QAbstractSpinBox*>(w);

        if (!label && !btn && !grp && !combo && !lineEdit && !spin) {
            continue;
        }

        const QString widgetText = [&]() {
            if (label) return label->text();
            if (btn) return btn->text();
            if (grp) return grp->title();
            if (combo) return combo->currentText();
            if (lineEdit) return lineEdit->text();
            return QString();
        }();

        // 1. widget->width() >= widget->minimumSizeHint().width() and height
        const QSize minHint = w->minimumSizeHint();
        if (minHint.isValid()) {
            if (minHint.width() > 0 && w->width() < minHint.width()) {
                violations.append({className,
                                   w->objectName(),
                                   widgetText,
                                   contextDescription,
                                   QString(u"width (%1) < minimumSizeHint.width (%2)"_s)
                                       .arg(w->width())
                                       .arg(minHint.width())});
            }
            if (minHint.height() > 0 && w->height() < minHint.height()) {
                violations.append({className,
                                   w->objectName(),
                                   widgetText,
                                   contextDescription,
                                   QString(u"height (%1) < minimumSizeHint.height (%2)"_s)
                                       .arg(w->height())
                                       .arg(minHint.height())});
            }
        }

        const bool elide = w->property("jobprepElide").toBool();

        // 2. Non-wrapping QLabel and buttons: fontMetrics().horizontalAdvance(text) <= contentsRect().width()
        if (label) {
            if (label->wordWrap()) {
                const int reqH = label->heightForWidth(label->width());
                if (reqH > 0 && label->height() < reqH) {
                    violations.append({className,
                                       label->objectName(),
                                       label->text(),
                                       contextDescription,
                                       QString(u"height (%1) < heightForWidth (%2)"_s)
                                           .arg(label->height())
                                           .arg(reqH)});
                }
            } else if (!elide) {
                const QString text = stripMnemonic(label->text());
                if (!text.isEmpty() && !text.contains(u'\n')) {
                    const int adv = label->fontMetrics().horizontalAdvance(text);
                    if (adv > label->contentsRect().width()) {
                        violations.append({className,
                                           label->objectName(),
                                           text,
                                           contextDescription,
                                           QString(u"horizontalAdvance (%1) > contentsRect.width (%2)"_s)
                                               .arg(adv)
                                               .arg(label->contentsRect().width())});
                    }
                }
            }
        } else if (btn && !elide) {
            if (auto* tb = qobject_cast<QToolButton*>(btn)) {
                if (tb->toolButtonStyle() == Qt::ToolButtonIconOnly) {
                    continue;
                }
            }
            const QString text = stripMnemonic(btn->text());
            if (!text.isEmpty() && !text.contains(u'\n')) {
                const int adv = btn->fontMetrics().horizontalAdvance(text);
                if (adv > btn->contentsRect().width()) {
                    violations.append({className,
                                       btn->objectName(),
                                       text,
                                       contextDescription,
                                       QString(u"button text advance (%1) > contentsRect.width (%2)"_s)
                                           .arg(adv)
                                           .arg(btn->contentsRect().width())});
                }
            }
        } else if (grp && !elide) {
            const QString title = stripMnemonic(grp->title());
            if (!title.isEmpty()) {
                const int adv = grp->fontMetrics().horizontalAdvance(title);
                if (adv > grp->contentsRect().width()) {
                    violations.append({className,
                                       grp->objectName(),
                                       title,
                                       contextDescription,
                                       QString(u"groupbox title advance (%1) > contentsRect.width (%2)"_s)
                                           .arg(adv)
                                           .arg(grp->contentsRect().width())});
                }
            }
        }
    }
    return violations;
}

void tst_HiDpi::testPagesHiDpi() {
    updateProgress(u"testPagesHiDpi start"_s); // CI-DIAG
    auto& win = m_fix->window();
    win.show();
    JobPrep::UiTest::activateWindow(&win);
    QApplication::processEvents();

    const QList<QSize> sizes = {QSize(1100, 700), QSize(1000, 540)};
    const QList<ThemeMode> themes = {ThemeMode::Light, ThemeMode::Dark};

    QList<ClippingViolation> allViolations;

    for (const ThemeMode theme : themes) {
        win.themeManager().setThemeMode(theme);
        QApplication::processEvents();
        const QString themeName = (theme == ThemeMode::Light ? u"Light"_s : u"Dark"_s);

        for (const QSize& size : sizes) {
            win.resize(size);
            QApplication::processEvents();
            // Use the actual window size after resize (may be clamped by layout minimum).
            const QSize actualSize = win.size();
            qInfo("testPagesHiDpi: requested %dx%d, actual %dx%d (layout min %dx%d)",
                  size.width(), size.height(),
                  actualSize.width(), actualSize.height(),
                  win.minimumSizeHint().width(), win.minimumSizeHint().height());
            const QString sizeName = QString(u"%1x%2"_s).arg(actualSize.width()).arg(actualSize.height());

            // 5 main pages: 0=Home, 1=Study, 2=Jobs, 3=Calendar, 4=Settings
            for (int pageIdx = 0; pageIdx < 5; ++pageIdx) {
                updateProgress(QString(u"testPagesHiDpi page=%1 theme=%2 size=%3"_s).arg(pageIdx).arg(themeName, sizeName)); // CI-DIAG
                win.pageStack()->setCurrentIndex(pageIdx);
                QApplication::processEvents();
                const QString pageName = QString(u"Page_%1"_s).arg(pageIdx);

                const QString desc = QString(u"%1 | %2 | %3"_s).arg(pageName, themeName, sizeName);
                const auto v = checkClipping(win.pageStack()->currentWidget(), desc);
                allViolations.append(v);
            }
        }
    }

    for (const auto& viol : allViolations) {
        fprintf(stderr, "CLIPPING_VIOLATION: [%s] %s (%s) '%s': %s\n",
                qPrintable(viol.context),
                qPrintable(viol.widgetClass),
                qPrintable(viol.objectName),
                qPrintable(viol.text),
                qPrintable(viol.reason));
    }
    fflush(stderr);

    QCOMPARE(allViolations.count(), 0);
}

void tst_HiDpi::testDialogsHiDpi() {
    updateProgress(u"testDialogsHiDpi start"_s); // CI-DIAG
    auto& win = m_fix->window();
    auto& ctx = m_fix->context();

    const QList<ThemeMode> themes = {ThemeMode::Light, ThemeMode::Dark};

    QList<ClippingViolation> allViolations;

    for (const ThemeMode theme : themes) {
        win.themeManager().setThemeMode(theme);
        QApplication::processEvents();
        const QString themeName = (theme == ThemeMode::Light ? u"Light"_s : u"Dark"_s);

        // 1. ApplicationDialog (with all tabs)
        {
            updateProgress(QString(u"testDialogsHiDpi dialog=ApplicationDialog theme=%1 open"_s).arg(themeName)); // CI-DIAG
            JobPrep::Ui::Dialogs::ApplicationDialog dlg(ctx.applications(),
                                                        ctx.interviews(),
                                                        JobPrep::Ui::Dialogs::ApplicationDialog::Edit,
                                                        &win);
            // Load first application if exists
            const auto apps = ctx.applications().all();
            if (!apps.isEmpty()) {
                dlg.setApplication(apps.first());
            }
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QTest::qWait(50);

            auto* tabWidget = dlg.findChild<QTabWidget*>();
            const int tabCount = tabWidget ? tabWidget->count() : 1;

            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                dlg.resize(size);
                for (int t = 0; t < tabCount; ++t) {
                    updateProgress(QString(u"testDialogsHiDpi dialog=ApplicationDialog theme=%1 size=%2 tab=%3"_s) // CI-DIAG
                                       .arg(themeName, size == dlg.sizeHint() ? u"sizeHint"_s : u"minSizeHint"_s).arg(t)); // CI-DIAG
                    if (tabWidget) {
                        tabWidget->setCurrentIndex(t);
                    }
                    dlg.layout()->activate();
                    QCoreApplication::processEvents();

                    // CI-DIAG start
                    QString tabWidths;
                    auto* bar = tabWidget ? tabWidget->tabBar() : nullptr;
                    if (bar) {
                        for (int i = 0; i < bar->count(); ++i) {
                            if (i > 0) tabWidths += u","_s;
                            tabWidths += QString::number(bar->tabRect(i).width());
                        }
                    }
                    QString scrollBtnVis;
                    for (auto* btn : dlg.findChildren<QToolButton*>()) {
                        scrollBtnVis += QString(u"%1:visToDlg=%2 "_s)
                                            .arg(btn->objectName().isEmpty() ? btn->metaObject()->className() : qPrintable(btn->objectName()))
                                            .arg(btn->isVisibleTo(&dlg) ? 1 : 0);
                    }
                    qInfo("// CI-DIAG testDialogsHiDpi: dlg=ApplicationDialog tab=%d theme=%s "
                          "requested=(%d,%d) actual=(%d,%d) minSize=(%d,%d) minSizeHint=(%d,%d) layMin=(%d,%d) "
                          "tabWidgetW=%d tabBarW=%d tabBarSizeHint=(%d,%d) tabBarMinHint=(%d,%d) usesScroll=%d "
                          "tabRects=[%s] scrollBtns=[%s] style=%s",
                          t, qPrintable(themeName),
                          size.width(), size.height(),
                          dlg.width(), dlg.height(),
                          dlg.minimumSize().width(), dlg.minimumSize().height(),
                          dlg.minimumSizeHint().width(), dlg.minimumSizeHint().height(),
                          dlg.layout()->minimumSize().width(), dlg.layout()->minimumSize().height(),
                          tabWidget ? tabWidget->width() : -1,
                          bar ? bar->width() : -1,
                          bar ? bar->sizeHint().width() : -1, bar ? bar->sizeHint().height() : -1,
                          bar ? bar->minimumSizeHint().width() : -1, bar ? bar->minimumSizeHint().height() : -1,
                          bar ? (bar->usesScrollButtons() ? 1 : 0) : -1,
                          qPrintable(tabWidths), qPrintable(scrollBtnVis.trimmed()),
                          qPrintable(QApplication::style() ? QApplication::style()->name() : u"none"_s));
                    // CI-DIAG end

                    const QString desc = QString(u"ApplicationDialog tab %1 | %2 | req %3x%4 actual %5x%6"_s)
                                             .arg(t)
                                             .arg(themeName)
                                             .arg(size.width())
                                             .arg(size.height())
                                             .arg(dlg.width())
                                             .arg(dlg.height());
                    allViolations.append(checkClipping(&dlg, desc));
                }
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 2. InterviewDialog
        {
            updateProgress(QString(u"testDialogsHiDpi dialog=InterviewDialog theme=%1 open"_s).arg(themeName)); // CI-DIAG
            JobPrep::Ui::Dialogs::InterviewDialog dlg(ctx.applications(),
                                                      ctx.interviews(),
                                                      JobPrep::Ui::Dialogs::InterviewDialog::Create,
                                                      &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                updateProgress(QString(u"testDialogsHiDpi dialog=InterviewDialog theme=%1 size=%2"_s) // CI-DIAG
                                   .arg(themeName, size == dlg.sizeHint() ? u"sizeHint"_s : u"minSizeHint"_s)); // CI-DIAG
                dlg.resize(size);
                dlg.layout()->activate();
                QApplication::processEvents();
                const QString desc = QString(u"InterviewDialog | %1 | req %2x%3 actual %4x%5"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height())
                                         .arg(dlg.width())
                                         .arg(dlg.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 3. FollowUpDialog
        {
            updateProgress(QString(u"testDialogsHiDpi dialog=FollowUpDialog theme=%1 open"_s).arg(themeName)); // CI-DIAG
            JobPrep::Ui::Dialogs::FollowUpDialog dlg(ctx.applications(),
                                                     QDate(2026, 11, 1),
                                                     &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                updateProgress(QString(u"testDialogsHiDpi dialog=FollowUpDialog theme=%1 size=%2"_s) // CI-DIAG
                                   .arg(themeName, size == dlg.sizeHint() ? u"sizeHint"_s : u"minSizeHint"_s)); // CI-DIAG
                dlg.resize(size);
                dlg.layout()->activate();
                QApplication::processEvents();
                const QString desc = QString(u"FollowUpDialog | %1 | req %2x%3 actual %4x%5"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height())
                                         .arg(dlg.width())
                                         .arg(dlg.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 4. SessionDialog
        {
            updateProgress(QString(u"testDialogsHiDpi dialog=SessionDialog theme=%1 open"_s).arg(themeName)); // CI-DIAG
            JobPrep::Ui::Dialogs::SessionDialog dlg(ctx.sessions(),
                                                    ctx.topics(),
                                                    &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                updateProgress(QString(u"testDialogsHiDpi dialog=SessionDialog theme=%1 size=%2"_s) // CI-DIAG
                                   .arg(themeName, size == dlg.sizeHint() ? u"sizeHint"_s : u"minSizeHint"_s)); // CI-DIAG
                dlg.resize(size);
                dlg.layout()->activate();
                QApplication::processEvents();
                const QString desc = QString(u"SessionDialog | %1 | req %2x%3 actual %4x%5"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height())
                                         .arg(dlg.width())
                                         .arg(dlg.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 5. TrackDialog
        {
            updateProgress(QString(u"testDialogsHiDpi dialog=TrackDialog theme=%1 open"_s).arg(themeName)); // CI-DIAG
            JobPrep::Ui::Dialogs::TrackDialog dlg(ctx.tracks(),
                                                  ctx.topics(),
                                                  &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                updateProgress(QString(u"testDialogsHiDpi dialog=TrackDialog theme=%1 size=%2"_s) // CI-DIAG
                                   .arg(themeName, size == dlg.sizeHint() ? u"sizeHint"_s : u"minSizeHint"_s)); // CI-DIAG
                dlg.resize(size);
                dlg.layout()->activate();
                QApplication::processEvents();
                const QString desc = QString(u"TrackDialog | %1 | req %2x%3 actual %4x%5"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height())
                                         .arg(dlg.width())
                                         .arg(dlg.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }
    }

    for (const auto& viol : allViolations) {
        fprintf(stderr, "CLIPPING_VIOLATION: [%s] %s (%s) '%s': %s\n",
                qPrintable(viol.context),
                qPrintable(viol.widgetClass),
                qPrintable(viol.objectName),
                qPrintable(viol.text),
                qPrintable(viol.reason));
    }
    fflush(stderr);

    QCOMPARE(allViolations.count(), 0);
}

void tst_HiDpi::testDialogsFitSmallScreen() {
    updateProgress(u"testDialogsFitSmallScreen start"_s); // CI-DIAG
    constexpr int kMaxSmallScreenHeight = 648;
    auto* screen = QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 720);
    qInfo("primaryScreen availableGeometry: %dx%d (at %d,%d)",
          avail.width(), avail.height(), avail.x(), avail.y());

    auto findButtonRowBottom = [](QWidget* dlg) -> QPoint {
        auto* btnBox = dlg->findChild<QDialogButtonBox*>();
        if (btnBox) {
            return btnBox->mapToGlobal(QPoint(0, btnBox->height()));
        }
        int maxY = 0;
        QWidget* bottomBtn = nullptr;
        for (auto* btn : dlg->findChildren<QPushButton*>()) {
            if (!btn->isVisibleTo(dlg)) continue;
            const int y = btn->mapTo(dlg, QPoint(0, btn->height())).y();
            if (y > maxY) {
                maxY = y;
                bottomBtn = btn;
            }
        }
        if (bottomBtn) {
            return bottomBtn->mapToGlobal(QPoint(0, bottomBtn->height()));
        }
        return dlg->mapToGlobal(QPoint(0, dlg->height()));
    };

    auto& win = m_fix->window();
    auto& ctx = m_fix->context();

    // 1. ApplicationDialog
    {
        updateProgress(u"testDialogsFitSmallScreen dialog=ApplicationDialog open"_s); // CI-DIAG
        JobPrep::Ui::Dialogs::ApplicationDialog dlg(ctx.applications(),
                                                    ctx.interviews(),
                                                    JobPrep::Ui::Dialogs::ApplicationDialog::Create,
                                                    &win);
        dlg.show();
        JobPrep::UiTest::activateWindow(&dlg);
        QCoreApplication::processEvents();

        auto* tabWidget = dlg.findChild<QTabWidget*>();
        for (int t = 0; t < (tabWidget ? tabWidget->count() : 1); ++t) {
            updateProgress(QString(u"testDialogsFitSmallScreen dialog=ApplicationDialog tab=%1"_s).arg(t)); // CI-DIAG
            if (tabWidget) tabWidget->setCurrentIndex(t);
            QCoreApplication::processEvents();
            const QSize sh = dlg.sizeHint();
            const QSize msh = dlg.minimumSizeHint();
            const bool fits = msh.height() <= kMaxSmallScreenHeight;
            qInfo("SMALL_SCREEN_CHECK: ApplicationDialog tab=%d sizeHint=(%d,%d) minSizeHint=(%d,%d) fits648=%s",
                  t, sh.width(), sh.height(), msh.width(), msh.height(), fits ? "YES" : "NO");
        }

        const QPoint btnBottomGlobal = findButtonRowBottom(&dlg);
        qInfo("ApplicationDialog btnBottomGlobal.y=%d avail.bottom=%d", btnBottomGlobal.y(), avail.bottom());
        QVERIFY2(dlg.minimumSizeHint().height() <= kMaxSmallScreenHeight,
                 qPrintable(QString("ApplicationDialog minimumSizeHint.height %1 > %2")
                                .arg(dlg.minimumSizeHint().height()).arg(kMaxSmallScreenHeight)));
        QVERIFY2(btnBottomGlobal.y() <= avail.bottom(),
                 qPrintable(QString("ApplicationDialog button bottom %1 > available bottom %2")
                                .arg(btnBottomGlobal.y()).arg(avail.bottom())));
        dlg.close();
    }

    // 2. InterviewDialog
    {
        updateProgress(u"testDialogsFitSmallScreen dialog=InterviewDialog"_s); // CI-DIAG
        JobPrep::Ui::Dialogs::InterviewDialog dlg(ctx.applications(),
                                                  ctx.interviews(),
                                                  JobPrep::Ui::Dialogs::InterviewDialog::Create,
                                                  &win);
        dlg.show();
        JobPrep::UiTest::activateWindow(&dlg);
        QCoreApplication::processEvents();

        const QSize sh = dlg.sizeHint();
        const QSize msh = dlg.minimumSizeHint();
        const bool fits = msh.height() <= kMaxSmallScreenHeight;
        qInfo("SMALL_SCREEN_CHECK: InterviewDialog sizeHint=(%d,%d) minSizeHint=(%d,%d) fits648=%s",
              sh.width(), sh.height(), msh.width(), msh.height(), fits ? "YES" : "NO");

        const QPoint btnBottomGlobal = findButtonRowBottom(&dlg);
        qInfo("InterviewDialog btnBottomGlobal.y=%d avail.bottom=%d", btnBottomGlobal.y(), avail.bottom());
        QVERIFY2(msh.height() <= kMaxSmallScreenHeight,
                 qPrintable(QString("InterviewDialog minimumSizeHint.height %1 > %2")
                                .arg(msh.height()).arg(kMaxSmallScreenHeight)));
        QVERIFY2(btnBottomGlobal.y() <= avail.bottom(),
                 qPrintable(QString("InterviewDialog button bottom %1 > available bottom %2")
                                .arg(btnBottomGlobal.y()).arg(avail.bottom())));
        dlg.close();
    }

    // 3. FollowUpDialog
    {
        updateProgress(u"testDialogsFitSmallScreen dialog=FollowUpDialog"_s); // CI-DIAG
        JobPrep::Ui::Dialogs::FollowUpDialog dlg(ctx.applications(),
                                                 QDate(2026, 11, 1),
                                                 &win);
        dlg.show();
        JobPrep::UiTest::activateWindow(&dlg);
        QCoreApplication::processEvents();

        const QSize sh = dlg.sizeHint();
        const QSize msh = dlg.minimumSizeHint();
        const bool fits = msh.height() <= kMaxSmallScreenHeight;
        qInfo("SMALL_SCREEN_CHECK: FollowUpDialog sizeHint=(%d,%d) minSizeHint=(%d,%d) fits648=%s",
              sh.width(), sh.height(), msh.width(), msh.height(), fits ? "YES" : "NO");

        const QPoint btnBottomGlobal = findButtonRowBottom(&dlg);
        qInfo("FollowUpDialog btnBottomGlobal.y=%d avail.bottom=%d", btnBottomGlobal.y(), avail.bottom());
        QVERIFY2(msh.height() <= kMaxSmallScreenHeight,
                 qPrintable(QString("FollowUpDialog minimumSizeHint.height %1 > %2")
                                .arg(msh.height()).arg(kMaxSmallScreenHeight)));
        QVERIFY2(btnBottomGlobal.y() <= avail.bottom(),
                 qPrintable(QString("FollowUpDialog button bottom %1 > available bottom %2")
                                .arg(btnBottomGlobal.y()).arg(avail.bottom())));
        dlg.close();
    }

    // 4. SessionDialog
    {
        updateProgress(u"testDialogsFitSmallScreen dialog=SessionDialog"_s); // CI-DIAG
        JobPrep::Ui::Dialogs::SessionDialog dlg(ctx.sessions(),
                                                ctx.topics(),
                                                &win);
        dlg.show();
        JobPrep::UiTest::activateWindow(&dlg);
        QCoreApplication::processEvents();

        const QSize sh = dlg.sizeHint();
        const QSize msh = dlg.minimumSizeHint();
        const bool fits = msh.height() <= kMaxSmallScreenHeight;
        qInfo("SMALL_SCREEN_CHECK: SessionDialog sizeHint=(%d,%d) minSizeHint=(%d,%d) fits648=%s",
              sh.width(), sh.height(), msh.width(), msh.height(), fits ? "YES" : "NO");

        const QPoint btnBottomGlobal = findButtonRowBottom(&dlg);
        qInfo("SessionDialog btnBottomGlobal.y=%d avail.bottom=%d", btnBottomGlobal.y(), avail.bottom());
        QVERIFY2(msh.height() <= kMaxSmallScreenHeight,
                 qPrintable(QString("SessionDialog minimumSizeHint.height %1 > %2")
                                .arg(msh.height()).arg(kMaxSmallScreenHeight)));
        QVERIFY2(btnBottomGlobal.y() <= avail.bottom(),
                 qPrintable(QString("SessionDialog button bottom %1 > available bottom %2")
                                .arg(btnBottomGlobal.y()).arg(avail.bottom())));
        dlg.close();
    }

    // 5. TrackDialog
    {
        updateProgress(u"testDialogsFitSmallScreen dialog=TrackDialog"_s); // CI-DIAG
        JobPrep::Ui::Dialogs::TrackDialog dlg(ctx.tracks(),
                                              ctx.topics(),
                                              &win);
        dlg.show();
        JobPrep::UiTest::activateWindow(&dlg);
        QCoreApplication::processEvents();

        const QSize sh = dlg.sizeHint();
        const QSize msh = dlg.minimumSizeHint();
        const bool fits = msh.height() <= kMaxSmallScreenHeight;
        qInfo("SMALL_SCREEN_CHECK: TrackDialog sizeHint=(%d,%d) minSizeHint=(%d,%d) fits648=%s",
              sh.width(), sh.height(), msh.width(), msh.height(), fits ? "YES" : "NO");

        const QPoint btnBottomGlobal = findButtonRowBottom(&dlg);
        qInfo("TrackDialog btnBottomGlobal.y=%d avail.bottom=%d", btnBottomGlobal.y(), avail.bottom());
        QVERIFY2(msh.height() <= kMaxSmallScreenHeight,
                 qPrintable(QString("TrackDialog minimumSizeHint.height %1 > %2")
                                .arg(msh.height()).arg(kMaxSmallScreenHeight)));
        QVERIFY2(btnBottomGlobal.y() <= avail.bottom(),
                 qPrintable(QString("TrackDialog button bottom %1 > available bottom %2")
                                .arg(btnBottomGlobal.y()).arg(avail.bottom())));
        dlg.close();
    }
    updateProgress(u"testDialogsFitSmallScreen end"_s); // CI-DIAG
}

void tst_HiDpi::testScreenshotsHiDpi() {
    updateProgress(u"testScreenshotsHiDpi start"_s); // CI-DIAG
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) {
        QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    }
    QVERIFY(QDir().mkpath(dir));

    auto& win = m_fix->window();
    win.resize(1100, 700);
    win.show();
    JobPrep::UiTest::activateWindow(&win);
    QApplication::processEvents();

    const struct {
        int pageIdx;
        const char* name;
    } pages[] = {
        {0, "home"},
        {1, "study"},
        {2, "jobs"},
        {3, "calendar"},
        {4, "settings"}
    };

    const QList<ThemeMode> themes = {ThemeMode::Light, ThemeMode::Dark};

    for (const ThemeMode theme : themes) {
        win.themeManager().setThemeMode(theme);
        QApplication::processEvents();
        const QString themeName = (theme == ThemeMode::Light ? u"light"_s : u"dark"_s);

        // 5 main pages
        for (const auto& p : pages) {
            updateProgress(QString(u"testScreenshotsHiDpi page=%1 theme=%2"_s).arg(QString::fromLatin1(p.name), themeName)); // CI-DIAG
            win.pageStack()->setCurrentIndex(p.pageIdx);
            QApplication::processEvents();
            const QImage img = win.grab().toImage();
            QVERIFY(!img.isNull());
            const QString path = QString(u"%1/hidpi150-%2-%3.png"_s).arg(dir, QString::fromLatin1(p.name), themeName);
            QVERIFY(img.save(path));
        }

        // Application dialog
        {
            updateProgress(QString(u"testScreenshotsHiDpi dialog=ApplicationDialog theme=%1"_s).arg(themeName)); // CI-DIAG
            JobPrep::Ui::Dialogs::ApplicationDialog dlg(m_fix->context().applications(),
                                                        m_fix->context().interviews(),
                                                        JobPrep::Ui::Dialogs::ApplicationDialog::Edit,
                                                        &win);
            const auto apps = m_fix->context().applications().all();
            if (!apps.isEmpty()) {
                dlg.setApplication(apps.first());
            }
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();

            const QImage img = dlg.grab().toImage();
            QVERIFY(!img.isNull());
            const QString path = QString(u"%1/hidpi150-application-dialog-%2.png"_s).arg(dir, themeName);
            QVERIFY(img.save(path));
            dlg.close();
            QApplication::processEvents();
        }
    }
    updateProgress(u"testScreenshotsHiDpi end"_s); // CI-DIAG
}

static QString s_cfgToRemove;
static void removeConfigOnExit() {
    if (!s_cfgToRemove.isEmpty()) {
        QFile::remove(s_cfgToRemove);
    }
}

int main(int argc, char* argv[]) {
#if defined(Q_OS_WIN)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    updateProgress(u"main start"_s); // CI-DIAG
    const QByteArray platform = qgetenv("QT_QPA_PLATFORM");
    const QString cfgPath = u"jobprep_offscreen_1080p.json"_s;
    if (platform.isEmpty() || platform == "offscreen" || platform == "offscreen:size=1920x1080") {
        QFile cfg(cfgPath);
        if (cfg.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            cfg.write("{\"screens\":[{\"name\":\"Screen0\",\"x\":0,\"y\":0,\"width\":1920,\"height\":1080}]}");
            cfg.close();
            s_cfgToRemove = cfgPath;
            std::atexit(removeConfigOnExit);
            qputenv("QT_QPA_PLATFORM", QByteArray("offscreen:configfile=") + cfgPath.toLocal8Bit());
        }
    }
    updateProgress(u"main before QApplication"_s); // CI-DIAG
    QApplication app(argc, argv);
    updateProgress(u"main after QApplication"_s); // CI-DIAG
    tst_HiDpi tc;
    const int result = QTest::qExec(&tc, argc, argv);
    QFile::remove(cfgPath);
    return result;
}
#include "tst_HiDpi.moc"
