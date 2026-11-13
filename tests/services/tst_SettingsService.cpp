#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include "app/Localization.h"
#include "services/SettingsService.h"

using namespace Qt::StringLiterals;
using JobPrep::Services::SettingsService;

class tst_SettingsService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void defaultLeadMinutesReturns1440And60();
    void sanitizingRemovesNonPositiveDuplicatesAndSortsDescending();
    void emptyOrInvalidStoredValueFallsBackToDefault();
    void signalEmittedOncePerRealChange();
    void testLeadMinutesSanitizing();
    void testTrayHintShownKey();
    void testToDayOfWeekNormalization();
    void testWeeklyGoalSurvivesRestart();
    void testLanguageSettingRoundTrip();
    void testResolveLanguage_data();
    void testResolveLanguage();
    void testInstallLanguageRepeatedly();
    void testEnglishLocaleIndependentOfSystem();
    void testMainCallsInstallLanguageBeforeWidgets();
};

void tst_SettingsService::initTestCase() {
    QCoreApplication::setOrganizationName(u"JobPrepTest"_s);
    QCoreApplication::setApplicationName(u"JobPrepTest_SettingsService"_s);
}

void tst_SettingsService::defaultLeadMinutesReturns1440And60() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    const QList<int> expected{1440, 60};
    QCOMPARE(svc.interviewLeadMinutes(), expected);
}

void tst_SettingsService::sanitizingRemovesNonPositiveDuplicatesAndSortsDescending() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    svc.setInterviewLeadMinutes({60, -10, 1440, 0, 60, 120});

    const QList<int> expected{1440, 120, 60};
    QCOMPARE(svc.interviewLeadMinutes(), expected);
}

void tst_SettingsService::emptyOrInvalidStoredValueFallsBackToDefault() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    svc.setInterviewLeadMinutes({-5, 0});
    const QList<int> expected{1440, 60};
    QCOMPARE(svc.interviewLeadMinutes(), expected);
}

void tst_SettingsService::signalEmittedOncePerRealChange() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    QSignalSpy spy(&svc, &SettingsService::interviewLeadMinutesChanged);

    // 1. First real change: default {1440, 60} -> {120, 30}
    svc.setInterviewLeadMinutes({30, 120});
    QCOMPARE(spy.count(), 1);
    const QList<int> expected{120, 30};
    QCOMPARE(spy.takeFirst().at(0).value<QList<int>>(), expected);

    // 2. Setting equivalent list (unsorted, duplicates) should not emit signal
    svc.setInterviewLeadMinutes({120, 30, 30});
    QCOMPARE(spy.count(), 0);

    // 3. Second real change: {120, 30} -> {60}
    svc.setInterviewLeadMinutes({60});
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).value<QList<int>>(), QList<int>({60}));
}

void tst_SettingsService::testLeadMinutesSanitizing() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    // Empty list sanitizes to default {1440, 60}
    svc.setInterviewLeadMinutes({});
    QCOMPARE(svc.interviewLeadMinutes(), (QList<int>{1440, 60}));

    // Large custom lead minutes and mixed order
    svc.setInterviewLeadMinutes({15, 10080, 30, 2880});
    QCOMPARE(svc.interviewLeadMinutes(), (QList<int>{10080, 2880, 30, 15}));
}

void tst_SettingsService::testTrayHintShownKey() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        QSettings* qs = qsettings.get(); // CI-DIAG
        SettingsService svc(std::move(qsettings));
        QCOMPARE(svc.trayHintShown(), false);

        QSignalSpy spy(&svc, &SettingsService::trayHintShownChanged);
        svc.setTrayHintShown(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), true);
        QCOMPARE(svc.trayHintShown(), true);

        // Setting same value does not emit
        svc.setTrayHintShown(true);
        QCOMPARE(spy.count(), 1);

        qs->sync(); // CI-DIAG
        qInfo().noquote() << "CI-DIAG testTrayHintShownKey first scope status:" // CI-DIAG
                          << static_cast<int>(qs->status()) // CI-DIAG
                          << "size:" << QFileInfo(path).size(); // CI-DIAG
    }

    // CI-DIAG: File inspection after first scope ends
    { // CI-DIAG
        QFileInfo fi(path); // CI-DIAG
        qInfo().noquote() << "CI-DIAG testTrayHintShownKey file exists:" << fi.exists() << "size:" << fi.size(); // CI-DIAG
        QFile f(path); // CI-DIAG
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) { // CI-DIAG
            QTextStream in(&f); // CI-DIAG
            for (int i = 0; i < 20 && !in.atEnd(); ++i) { // CI-DIAG
                qInfo().noquote() << "CI-DIAG testTrayHintShownKey L" << (i + 1) << ":" << in.readLine(); // CI-DIAG
            } // CI-DIAG
        } // CI-DIAG
    } // CI-DIAG

    // Persists across SettingsService instances
    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        QSettings* qs = qsettings.get(); // CI-DIAG
        SettingsService svc(std::move(qsettings));
        qs->sync(); // CI-DIAG
        qInfo().noquote() << "CI-DIAG testTrayHintShownKey" // CI-DIAG
                          << "org:" << QCoreApplication::organizationName() // CI-DIAG
                          << "app:" << QCoreApplication::applicationName() // CI-DIAG
                          << "arg0:" << QCoreApplication::arguments().value(0) // CI-DIAG
                          << "fileName:" << qs->fileName() // CI-DIAG
                          << "format:" << static_cast<int>(qs->format()) // CI-DIAG
                          << "status:" << static_cast<int>(qs->status()) // CI-DIAG
                          << "writable:" << qs->isWritable() // CI-DIAG
                          << "value:" << qs->value(u"app/trayHintShown"_s); // CI-DIAG
        QCOMPARE(svc.trayHintShown(), true);
    }
}

void tst_SettingsService::testToDayOfWeekNormalization() {
    // 1 -> Monday, 7 -> Sunday
    QCOMPARE(SettingsService::toDayOfWeek(1), Qt::Monday);
    QCOMPARE(SettingsService::toDayOfWeek(7), Qt::Sunday);

    // Invalid / legacy values normalize to Monday
    QCOMPARE(SettingsService::toDayOfWeek(0), Qt::Monday);
    QCOMPARE(SettingsService::toDayOfWeek(2), Qt::Monday);
    QCOMPARE(SettingsService::toDayOfWeek(3), Qt::Monday);
    QCOMPARE(SettingsService::toDayOfWeek(8), Qt::Monday);
    QCOMPARE(SettingsService::toDayOfWeek(-1), Qt::Monday);
}

void tst_SettingsService::testWeeklyGoalSurvivesRestart() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        QSettings* qs = qsettings.get(); // CI-DIAG
        SettingsService svc(std::move(qsettings));
        QCOMPARE(svc.weeklyGoalMinutes(), 360);
        svc.setWeeklyGoalMinutes(480);
        QCOMPARE(svc.weeklyGoalMinutes(), 480);

        qs->sync(); // CI-DIAG
        qInfo().noquote() << "CI-DIAG testWeeklyGoalSurvivesRestart first scope status:" // CI-DIAG
                          << static_cast<int>(qs->status()) // CI-DIAG
                          << "size:" << QFileInfo(path).size(); // CI-DIAG
    }

    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        SettingsService svc(std::move(qsettings));
        QCOMPARE(svc.weeklyGoalMinutes(), 480);
    }
}

void tst_SettingsService::testLanguageSettingRoundTrip() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(u"settings.ini"_s);

    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        SettingsService svc(std::move(qsettings));

        // Default value is "system"
        QCOMPARE(svc.language(), u"system"_s);

        QSignalSpy spy(&svc, &SettingsService::languageChanged);

        // Valid values
        svc.setLanguage(u"uk"_s);
        QCOMPARE(svc.language(), u"uk"_s);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), u"uk"_s);

        // Same value does not emit
        svc.setLanguage(u"uk"_s);
        QCOMPARE(spy.count(), 0);

        svc.setLanguage(u"ru"_s);
        QCOMPARE(svc.language(), u"ru"_s);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        svc.setLanguage(u"en"_s);
        QCOMPARE(svc.language(), u"en"_s);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        // Unknown value normalizes to "system"
        svc.setLanguage(u"fr_FR"_s);
        QCOMPARE(svc.language(), u"system"_s);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        // Empty value normalizes to "system" (already "system", so no change signal)
        svc.setLanguage(QString());
        QCOMPARE(svc.language(), u"system"_s);
        QCOMPARE(spy.count(), 0);
    }

    // Persistence across restart
    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        SettingsService svc(std::move(qsettings));
        svc.setLanguage(u"uk"_s);
    }
    {
        auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
        SettingsService svc(std::move(qsettings));
        QCOMPARE(svc.language(), u"uk"_s);
    }
}

void tst_SettingsService::testResolveLanguage_data() {
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QStringList>("uiLanguages");
    QTest::addColumn<QString>("expected");

    // System setting evaluates uiLanguages in order
    QTest::newRow("system uk-UA") << u"system"_s << QStringList{u"uk-UA"_s} << u"uk"_s;
    QTest::newRow("system uk") << u"system"_s << QStringList{u"uk"_s} << u"uk"_s;
    QTest::newRow("system ru-RU") << u"system"_s << QStringList{u"ru-RU"_s} << u"ru"_s;
    QTest::newRow("system ru-UA") << u"system"_s << QStringList{u"ru-UA"_s} << u"ru"_s;
    QTest::newRow("system en-US") << u"system"_s << QStringList{u"en-US"_s} << u"en"_s;
    QTest::newRow("system pl-PL") << u"system"_s << QStringList{u"pl-PL"_s} << u"en"_s;
    QTest::newRow("system be-BY") << u"system"_s << QStringList{u"be-BY"_s} << u"en"_s;
    QTest::newRow("system empty list") << u"system"_s << QStringList{} << u"en"_s;
    QTest::newRow("system pl-PL then uk-UA")
        << u"system"_s << QStringList{u"pl-PL"_s, u"uk-UA"_s} << u"uk"_s;

    // Explicit setting overrides any system UI languages
    QTest::newRow("explicit en with uk-UA") << u"en"_s << QStringList{u"uk-UA"_s} << u"en"_s;
    QTest::newRow("explicit uk with en-US") << u"uk"_s << QStringList{u"en-US"_s} << u"uk"_s;
    QTest::newRow("explicit ru with en-US") << u"ru"_s << QStringList{u"en-US"_s} << u"ru"_s;
    QTest::newRow("unknown setting with uk-UA") << u"unknown"_s << QStringList{u"uk-UA"_s} << u"uk"_s;
}

void tst_SettingsService::testResolveLanguage() {
    QFETCH(QString, setting);
    QFETCH(QStringList, uiLanguages);
    QFETCH(QString, expected);

    QCOMPARE(JobPrep::App::resolveLanguage(setting, uiLanguages), expected);
}

void tst_SettingsService::testInstallLanguageRepeatedly() {
    // 1. Ukrainian
    auto resUk = JobPrep::App::installLanguage(u"uk"_s);
    QCOMPARE(resUk.language, u"uk"_s);
    QCOMPARE(QLocale().language(), QLocale::Ukrainian);
    QVERIFY(resUk.appTranslatorLoaded);

    // 2. Russian
    auto resRu = JobPrep::App::installLanguage(u"ru"_s);
    QCOMPARE(resRu.language, u"ru"_s);
    QCOMPARE(QLocale().language(), QLocale::Russian);
    QVERIFY(resRu.appTranslatorLoaded);

    // 3. English (restores default without app translator)
    auto resEn = JobPrep::App::installLanguage(u"en"_s);
    QCOMPARE(resEn.language, u"en"_s);
    QCOMPARE(QLocale().language(), QLocale::English);
    QVERIFY(!resEn.appTranslatorLoaded);

    // 4. Fallback when given invalid language
    auto resInvalid = JobPrep::App::installLanguage(u"unknown_lang"_s);
    QCOMPARE(resInvalid.language, u"en"_s);

    // Cleanup
    JobPrep::App::uninstallLanguage();
}

void tst_SettingsService::testEnglishLocaleIndependentOfSystem() {
    // Save original default locale
    const QLocale savedLocale = QLocale();
    struct LocaleGuard {
        QLocale orig;
        ~LocaleGuard() {
            QLocale::setDefault(orig);
            JobPrep::App::uninstallLanguage();
        }
    } guard{savedLocale};

    // 1. Simulate non-English system locale (German)
    QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
    QCOMPARE(QLocale().language(), QLocale::German);

    // 2. Install English
    auto res = JobPrep::App::installLanguage(u"en"_s);
    QCOMPARE(res.language, u"en"_s);
    QCOMPARE(QLocale().language(), QLocale::English);

    // 3. Verify month title is in English ("January 2026", not German "Januar 2026")
    const QString monthTitle = QLocale().toString(QDate(2026, 1, 1), u"MMMM yyyy"_s);
    QCOMPARE(monthTitle, u"January 2026"_s);

    // 4. Verify day abbreviation is English ("Mon", not German "Mo")
    const QString shortDay = QLocale().dayName(1, QLocale::ShortFormat);
    QCOMPARE(shortDay, u"Mon"_s);
}

void tst_SettingsService::testMainCallsInstallLanguageBeforeWidgets() {
    QString mainPath;
    const QStringList candidates{
        u"src/main.cpp"_s,
        u"../src/main.cpp"_s,
        u"../../src/main.cpp"_s,
        u"../../../src/main.cpp"_s,
#ifdef QT_TESTCASE_SOURCEDIR
        QDir(QString::fromUtf8(QT_TESTCASE_SOURCEDIR)).filePath(u"../../src/main.cpp"_s)
#endif
    };
    for (const QString& cand : candidates) {
        if (!cand.isEmpty() && QFile::exists(cand)) {
            mainPath = cand;
            break;
        }
    }
    QVERIFY2(!mainPath.isEmpty(), "Could not locate src/main.cpp");
    QFile mainFile(mainPath);
    QVERIFY2(mainFile.open(QIODevice::ReadOnly | QIODevice::Text), "Could not open src/main.cpp");

    const QString content = QString::fromUtf8(mainFile.readAll());
    const int installPos = content.indexOf(u"JobPrep::App::installLanguage"_s);
    const int appIconPos = content.indexOf(u"QApplication::setWindowIcon"_s);
    const int ctxPos = content.indexOf(u"JobPrep::App::AppContext ctx"_s);
    const int msgBoxPos = content.indexOf(u"QMessageBox::critical"_s);
    const int windowPos = content.indexOf(u"mainWindow(ctx"_s);

    QVERIFY2(installPos != -1, "JobPrep::App::installLanguage not found in main.cpp");
    QVERIFY2(appIconPos != -1, "QApplication::setWindowIcon not found in main.cpp");
    QVERIFY2(ctxPos != -1, "AppContext not found in main.cpp");
    QVERIFY2(msgBoxPos != -1, "QMessageBox::critical not found in main.cpp");
    QVERIFY2(windowPos != -1, "mainWindow(ctx not found in main.cpp");

    QVERIFY2(installPos < appIconPos, "installLanguage must precede window icon setup");
    QVERIFY2(installPos < ctxPos, "installLanguage must precede AppContext creation");
    QVERIFY2(installPos < msgBoxPos, "installLanguage must precede QMessageBox::critical");
    QVERIFY2(installPos < windowPos, "installLanguage must precede mainWindow creation");
}

QTEST_GUILESS_MAIN(tst_SettingsService)
#include "tst_SettingsService.moc"
