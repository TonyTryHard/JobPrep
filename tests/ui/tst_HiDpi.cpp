#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QDate>
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
    void testScreenshotsHiDpi();

private:
    QList<ClippingViolation> checkClipping(QWidget* root, const QString& contextDescription);
    std::unique_ptr<JobPrep::UiTest::WindowFixture> m_fix;
    QTemporaryDir m_settingsDir;
};

void tst_HiDpi::initTestCase() {
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
}

void tst_HiDpi::cleanupTestCase() {
    m_fix.reset();
    JobPrep::UiTest::uninstallWarningFilter();
}

void tst_HiDpi::testDevicePixelRatio() {
    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const qreal dpr = screen->devicePixelRatio();
    fprintf(stdout, "DPI_RATIO: %f\n", static_cast<double>(dpr));
    fflush(stdout);
    // When QT_SCALE_FACTOR=1.5 is set, screen->devicePixelRatio() is 1.5
    // Allow small epsilon for floating point comparison
    QVERIFY(qAbs(dpr - 1.5) < 0.05 || qAbs(dpr - 1.0) < 0.05 || qAbs(dpr - 2.0) < 0.05);
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

        // 1. widget->width() >= widget->minimumSizeHint().width() and height
        const QSize minHint = w->minimumSizeHint();
        if (minHint.isValid()) {
            if (minHint.width() > 0 && w->width() < minHint.width()) {
                violations.append({className,
                                   w->objectName(),
                                   {},
                                   contextDescription,
                                   QString(u"width (%1) < minimumSizeHint.width (%2)"_s)
                                       .arg(w->width())
                                       .arg(minHint.width())});
            }
            if (minHint.height() > 0 && w->height() < minHint.height()) {
                violations.append({className,
                                   w->objectName(),
                                   {},
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
        }
    }
    return violations;
}

void tst_HiDpi::testPagesHiDpi() {
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
            const QString sizeName = QString(u"%1x%2"_s).arg(size.width()).arg(size.height());

            // 5 main pages: 0=Home, 1=Study, 2=Jobs, 3=Calendar, 4=Settings
            for (int pageIdx = 0; pageIdx < 5; ++pageIdx) {
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
            QApplication::processEvents();

            auto* tabWidget = dlg.findChild<QTabWidget*>();
            const int tabCount = tabWidget ? tabWidget->count() : 1;

            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                dlg.resize(size);
                QApplication::processEvents();
                for (int t = 0; t < tabCount; ++t) {
                    if (tabWidget) {
                        tabWidget->setCurrentIndex(t);
                        QApplication::processEvents();
                    }
                    const QString desc = QString(u"ApplicationDialog tab %1 | %2 | %3x%4"_s)
                                             .arg(t)
                                             .arg(themeName)
                                             .arg(size.width())
                                             .arg(size.height());
                    allViolations.append(checkClipping(&dlg, desc));
                }
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 2. InterviewDialog
        {
            JobPrep::Ui::Dialogs::InterviewDialog dlg(ctx.applications(),
                                                      ctx.interviews(),
                                                      JobPrep::Ui::Dialogs::InterviewDialog::Create,
                                                      &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                dlg.resize(size);
                QApplication::processEvents();
                const QString desc = QString(u"InterviewDialog | %1 | %2x%3"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 3. FollowUpDialog
        {
            JobPrep::Ui::Dialogs::FollowUpDialog dlg(ctx.applications(),
                                                     QDate(2026, 11, 1),
                                                     &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                dlg.resize(size);
                QApplication::processEvents();
                const QString desc = QString(u"FollowUpDialog | %1 | %2x%3"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 4. SessionDialog
        {
            JobPrep::Ui::Dialogs::SessionDialog dlg(ctx.sessions(),
                                                    ctx.topics(),
                                                    &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                dlg.resize(size);
                QApplication::processEvents();
                const QString desc = QString(u"SessionDialog | %1 | %2x%3"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height());
                allViolations.append(checkClipping(&dlg, desc));
            }
            dlg.close();
            QApplication::processEvents();
        }

        // 5. TrackDialog
        {
            JobPrep::Ui::Dialogs::TrackDialog dlg(ctx.tracks(),
                                                  ctx.topics(),
                                                  &win);
            dlg.show();
            JobPrep::UiTest::activateWindow(&dlg);
            QApplication::processEvents();
            const QList<QSize> dlgSizes = {dlg.sizeHint(), dlg.minimumSizeHint()};
            for (const auto& size : dlgSizes) {
                dlg.resize(size);
                QApplication::processEvents();
                const QString desc = QString(u"TrackDialog | %1 | %2x%3"_s)
                                         .arg(themeName)
                                         .arg(size.width())
                                         .arg(size.height());
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

void tst_HiDpi::testScreenshotsHiDpi() {
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
            win.pageStack()->setCurrentIndex(p.pageIdx);
            QApplication::processEvents();
            const QImage img = win.grab().toImage();
            QVERIFY(!img.isNull());
            const QString path = QString(u"%1/hidpi150-%2-%3.png"_s).arg(dir, QString::fromLatin1(p.name), themeName);
            QVERIFY(img.save(path));
        }

        // Application dialog
        {
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
}

QTEST_MAIN(tst_HiDpi)
#include "tst_HiDpi.moc"
