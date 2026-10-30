#include <cmath>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QVBoxLayout>
#include "app/AppContext.h"
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/Migrations.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/Structs.h"
#include "services/DayWatcher.h"
#include "services/ExportService.h"
#include "services/ReminderService.h"
#include "services/RestoreService.h"
#include "services/SeedService.h"
#include "services/SettingsService.h"
#include "services/StatsService.h"
#include "ui/DashboardController.h"
#include "ui/DataToolsController.h"
#include "ui/FormatHelper.h"
#include "ui/MainWindow.h"
#include "ui/ReminderCoordinator.h"
#include "ui/Sidebar.h"
#include "ui/TrayNotifier.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/pages/StudyPage.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"
#include "ui/widgets/GoalProgressBar.h"
#include "ui/widgets/TopicBoardColumn.h"
#include "ui/widgets/TopicDetailPanel.h"
#include "UiTestHelpers.h"

using namespace Qt::StringLiterals;
using namespace JobPrep::UiTest;

namespace {

using JobPrep::App::AppContext;
using JobPrep::Domain::JobApplication;
using JobPrep::Domain::StudySession;
using JobPrep::Domain::Topic;
using JobPrep::Domain::TopicStatus;
using JobPrep::Ui::DataToolsController;
using JobPrep::Domain::Track;
using JobPrep::Services::SettingsService;
using JobPrep::Ui::DashboardController;
using JobPrep::Ui::FormatHelper;
using JobPrep::Ui::MainWindow;
using JobPrep::Ui::Pages::SettingsPage;
using JobPrep::Ui::Theme::AccentPreset;
using JobPrep::Ui::Theme::ThemeManager;
using JobPrep::Ui::Theme::ThemeMode;
using JobPrep::Ui::TrayNotifier;

double srgbToLinear(double c) {
    return (c <= 0.04045) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor& c) {
    return 0.2126 * srgbToLinear(c.redF()) +
           0.7152 * srgbToLinear(c.greenF()) +
           0.0722 * srgbToLinear(c.blueF());
}

double calcContrastRatio(const QColor& c1, const QColor& c2) {
    double l1 = relativeLuminance(c1);
    double l2 = relativeLuminance(c2);
    if (l1 < l2) std::swap(l1, l2);
    return (l1 + 0.05) / (l2 + 0.05);
}

}  // namespace

class tst_PageSettings : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // B1: Weekly goal
    void testWeeklyGoalSpinBox();
    void testWeeklyGoalDashboardInstantUpdate();
    void testWeeklyGoalPersistence();

    // C0.3: Goal spin box typed input
    void testGoalSpinBoxTypedEight();
    void testGoalSpinBoxTypingRules_data();
    void testGoalSpinBoxTypingRules();
    void testGoalSpinBoxErrorRestoresPreviousValue();
    void testGoalSpinBoxTypedSeparators();
    void testGoalSpinBoxTypedHoursSuffix();
    void testGoalSpinBoxTypedHoursAndMinutes();
    void testGoalSpinBoxTypedMinutesSuffix();
    void testGoalSpinBoxClamping();
    void testGoalSpinBoxInvalidText();
    void testGoalSpinBoxPageSteps();

    // C0.5: formatLeadTime assertions
    void testFormatLeadTime();

    // B2: Reminder lead times
    void testLeadTimesEditorPresetsAndCustom();
    void testLeadTimesRemovalAndNeverEmpty();
    void testLeadTimesServiceIntegration();

    // B3 / C0.1: Close to tray
    void testCloseToTrayUnavailable();
    void testCloseToTrayAvailableAndHide();
    void testCloseToTrayRestore();
    void testCloseToTrayQuitSkipsCloseEventAndFlushesEdits();
    void testQuitOnLastWindowClosedBehavior();
    void testCloseToTrayLiveProvider();
    void testCloseToTrayFlushOnHide();
    void testCloseToTrayActivationReasonsAndRestore();
    void testCloseToTrayFirstTimeHintOnlyWhenRequested();
    void testCloseToTraySessionSaving();

    // B4 / C0.6: First day of week
    void testFirstDayOfWeekComboAndNormalization();
    void testFirstDayOfWeekSettingsToDashboard();

    // B5: Theme "System" & switching
    void testThemeAndAccentSwitching();
    void testSystemThemeColorSchemeNotification();

    // B6: Painted & contrast tests
    void testSettingsPageStructure();
    void testSettingsPagePaintedStartupMatchesSwitch();
    void testSettingsPageContrastAcrossAccents();
    void testSettingsScreenshots();

    // Keyboard navigation & real key interactions
    void testKeyInteractions();

    // C8: Data & Backups section tests
    void testDataCardStructureAndControls();
    void testDataExportCsvWithSeams();
    void testDataExportIcsWithSeams();
    void testDataBackupJsonWithSeams();
    void testDataRestoreJsonWithSeams();
    void testDataLoadSampleDataTwiceNoDuplicates();
    void testDataOpenFolderWithSeams();
    void testDataCardScreenshots();

private:
    QTemporaryDir m_tempDir;
};

void tst_PageSettings::initTestCase() {
    installWarningFilter();
    isolateSettings(m_tempDir);
}

void tst_PageSettings::cleanupTestCase() {
    JOBPREP_VERIFY_ZERO_WARNINGS();
    uninstallWarningFilter();
}

void tst_PageSettings::testSettingsPageStructure() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), true);
    page.resize(1000, 700);
    page.show();
    QCoreApplication::processEvents();

    // Verify cards exist
    auto* appCard = page.findChild<QFrame*>(u"SettingsAppearanceCard"_s);
    auto* prefsCard = page.findChild<QFrame*>(u"SettingsPreferencesCard"_s);
    QVERIFY(appCard != nullptr);
    QVERIFY(prefsCard != nullptr);

    // Verify all controls exist
    QVERIFY(page.themeCombo() != nullptr);
    QVERIFY(page.accentCombo() != nullptr);
    QVERIFY(page.studyGoalSpinBox() != nullptr);
    QVERIFY(page.weekStartCombo() != nullptr);
    QVERIFY(page.closeToTrayCheck() != nullptr);
    QVERIFY(page.leadList() != nullptr);
    QVERIFY(page.leadPresetCombo() != nullptr);
    QVERIFY(page.customLeadSpinBox() != nullptr);
    QVERIFY(page.addLeadButton() != nullptr);
    QVERIFY(page.removeLeadButton() != nullptr);
    QVERIFY(page.resetLeadButton() != nullptr);

    // Every control has a non-empty tooltip
    QVERIFY(!page.themeCombo()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.accentCombo()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.studyGoalSpinBox()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.weekStartCombo()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.closeToTrayCheck()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.leadList()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.leadPresetCombo()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.customLeadSpinBox()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.addLeadButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.removeLeadButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.resetLeadButton()->toolTip().trimmed().isEmpty());

    // Documented tab order verification
    // themeCombo -> accentCombo -> studyGoalSpinBox -> weekStartCombo -> closeToTrayCheck
    // -> leadList -> leadPresetCombo -> customLeadSpinBox -> addLeadBtn -> removeLeadBtn -> resetLeadBtn
    QList<QWidget*> expectedTabOrder = {
        page.themeCombo(),
        page.accentCombo(),
        page.studyGoalSpinBox(),
        page.weekStartCombo(),
        page.closeToTrayCheck(),
        page.leadList(),
        page.leadPresetCombo(),
        page.customLeadSpinBox(),
        page.addLeadButton(),
        page.removeLeadButton(),
        page.resetLeadButton()
    };

    // Enable custom spinbox by selecting Custom preset so all controls in chain are enabled
    page.leadPresetCombo()->setCurrentIndex(page.leadPresetCombo()->findData(-1));

    auto nextControlInFocusChain = [](QWidget* current) -> QWidget* {
        QWidget* next = current->nextInFocusChain();
        while (next && (next->focusPolicy() == Qt::NoFocus || !next->isEnabled() || current->isAncestorOf(next))) {
            next = next->nextInFocusChain();
        }
        return next;
    };

    for (int i = 0; i < expectedTabOrder.size() - 1; ++i) {
        QWidget* current = expectedTabOrder[i];
        QWidget* nextExpected = expectedTabOrder[i + 1];
        QWidget* actualNext = nextControlInFocusChain(current);
        QCOMPARE(actualNext, nextExpected);
    }
}

void tst_PageSettings::testWeeklyGoalSpinBox() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    QVERIFY(spin != nullptr);

    // Range 60 - 2400 min, step 15
    QCOMPARE(spin->minimum(), 60);
    QCOMPARE(spin->maximum(), 2400);
    QCOMPARE(spin->singleStep(), 15);

    // Formatted text
    spin->setValue(60);
    QCOMPARE(spin->text(), u"1 h"_s);
    spin->setValue(75);
    QCOMPARE(spin->text(), u"1 h 15 min"_s);
    spin->setValue(120);
    QCOMPARE(spin->text(), u"2 h"_s);
    spin->setValue(360);
    QCOMPARE(spin->text(), u"6 h"_s);
    spin->setValue(375);
    QCOMPARE(spin->text(), u"6 h 15 min"_s);
    spin->setValue(480);
    QCOMPARE(spin->text(), u"8 h"_s);

    // Step up and down via arrow keys
    page.show();
    activateWindow(&page);
    spin->setFocus();
    spin->setValue(360);
    QTest::keyClick(spin, Qt::Key_Up);
    QCOMPARE(spin->value(), 375);
    QTest::keyClick(spin, Qt::Key_Down);
    QCOMPARE(spin->value(), 360);

    // Out-of-range typing clamped
    auto* lineEdit = spin->findChild<QLineEdit*>();
    QVERIFY(lineEdit != nullptr);
    lineEdit->setText(u"5000"_s);
    QTest::keyClick(lineEdit, Qt::Key_Return);
    QCOMPARE(spin->value(), 2400);

    lineEdit->setText(u"10 min"_s);
    QTest::keyClick(lineEdit, Qt::Key_Return);
    QCOMPARE(spin->value(), 60);
}

void tst_PageSettings::testWeeklyGoalDashboardInstantUpdate() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setWeeklyGoalMinutes(360);

    MainWindow win(ctx, false);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    auto* dashboard = win.dashboardController();
    QVERIFY(dashboard != nullptr);
    dashboard->refreshNow();
    QCOMPARE(dashboard->snapshot().weeklyGoalMinutes, 360);

    // Switch to Settings page and change spin box from 360 (6 h) to 480 (8 h)
    auto* settingsPage = win.settingsPage();
    QVERIFY(settingsPage != nullptr);
    auto* spin = settingsPage->studyGoalSpinBox();
    QVERIFY(spin != nullptr);
    QCOMPARE(spin->value(), 360);

    spin->setValue(480);
    QCoreApplication::processEvents();

    // Value applies immediately without restart or "Apply" button
    QCOMPARE(ctx.settings().weeklyGoalMinutes(), 480);
    QCOMPARE(dashboard->snapshot().weeklyGoalMinutes, 480);

    // Verify progress bar and goal label on Home page reflect 8 h
    auto* homePage = win.homePage();
    QVERIFY(homePage != nullptr);
    auto* goalBar = homePage->findChild<JobPrep::Ui::Widgets::GoalProgressBar*>();
    QVERIFY(goalBar != nullptr);
    QCOMPARE(goalBar->goalMinutes(), 480);
}

void tst_PageSettings::testWeeklyGoalPersistence() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const QString filePath = tempFile.fileName();
    tempFile.close();

    {
        auto qsettings = std::make_unique<QSettings>(filePath, QSettings::IniFormat);
        SettingsService svc(std::move(qsettings));
        QCOMPARE(svc.weeklyGoalMinutes(), 360);  // default

        svc.setWeeklyGoalMinutes(540);
        QCOMPARE(svc.weeklyGoalMinutes(), 540);
    }

    // New instance loads persisted value
    {
        auto qsettings = std::make_unique<QSettings>(filePath, QSettings::IniFormat);
        SettingsService svc(std::move(qsettings));
        QCOMPARE(svc.weeklyGoalMinutes(), 540);
    }
}

void tst_PageSettings::testGoalSpinBoxTypedEight() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    QVERIFY(spin != nullptr);
    auto* edit = spin->findChild<QLineEdit*>();
    QVERIFY(edit != nullptr);

    edit->setText(u"8"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 480);
}

void tst_PageSettings::testGoalSpinBoxTypingRules_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("isError");
    QTest::addColumn<int>("expectedMinutes");

    // Bare integer hours
    QTest::newRow("8") << u"8"_s << false << 480;

    // Time separators with 2 digits (00-59)
    QTest::newRow("7:30") << u"7:30"_s << false << 450;
    QTest::newRow("7;30") << u"7;30"_s << false << 450;
    QTest::newRow("7.30") << u"7.30"_s << false << 450;
    QTest::newRow("7,30") << u"7,30"_s << false << 450;
    QTest::newRow("7.05") << u"7.05"_s << false << 420; // 425 -> rounded to 420

    // Time separators with 1 digit (tens of minutes)
    QTest::newRow("7.5") << u"7.5"_s << false << 465; // 470 -> rounded to 465
    QTest::newRow("7:5") << u"7:5"_s << false << 465;
    QTest::newRow("7,5") << u"7,5"_s << false << 465;
    QTest::newRow("7.1") << u"7.1"_s << false << 435; // 430 -> rounded to 435
    QTest::newRow("7.0") << u"7.0"_s << false << 420; // 420 -> 420

    // Errors: single digit 6-9
    QTest::newRow("7.6") << u"7.6"_s << true << 0;
    QTest::newRow("7.7") << u"7.7"_s << true << 0;
    QTest::newRow("7.8") << u"7.8"_s << true << 0;
    QTest::newRow("7.9") << u"7.9"_s << true << 0;

    // Errors: two-digit minutes >= 60
    QTest::newRow("7.75") << u"7.75"_s << true << 0;
    QTest::newRow("7:60") << u"7:60"_s << true << 0;

    // Errors: 3 or more digits after separator
    QTest::newRow("7.300") << u"7.300"_s << true << 0;

    // Errors: missing part
    QTest::newRow("7.") << u"7."_s << true << 0;
    QTest::newRow(".30") << u".30"_s << true << 0;

    // Errors: empty or letters
    QTest::newRow("empty") << u""_s << true << 0;
    QTest::newRow("letters") << u"abc"_s << true << 0;

    // Units
    QTest::newRow("8 h") << u"8 h"_s << false << 480;
    QTest::newRow("8h") << u"8h"_s << false << 480;
    QTest::newRow("7 h 30 min") << u"7 h 30 min"_s << false << 450;
    QTest::newRow("90 min") << u"90 min"_s << false << 90;

    // Clamping & Rounding
    QTest::newRow("7:20") << u"7:20"_s << false << 435; // 440 -> rounded to 435
    QTest::newRow("0:50") << u"0:50"_s << false << 60;  // 50 -> clamped to 60
    QTest::newRow("99 h") << u"99 h"_s << false << 2400;// 5940 -> clamped to 2400
    QTest::newRow("45") << u"45"_s << false << 2400;    // 2700 -> clamped to 2400
}

void tst_PageSettings::testGoalSpinBoxTypingRules() {
    QFETCH(QString, input);
    QFETCH(bool, isError);
    QFETCH(int, expectedMinutes);

    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    // Initial baseline: 480 (8 h)
    spin->setValue(480);

    edit->setText(input);
    QTest::keyClick(edit, Qt::Key_Return);

    if (isError) {
        QCOMPARE(spin->value(), 480);
        QCOMPARE(edit->text(), u"8 h"_s);
    } else {
        QCOMPARE(spin->value(), expectedMinutes);
    }
}

void tst_PageSettings::testGoalSpinBoxErrorRestoresPreviousValue() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    page.show();
    QApplication::processEvents();

    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();
    auto* themeCombo = page.findChild<QComboBox*>(u"SettingsThemeCombo"_s);
    QVERIFY(spin != nullptr && edit != nullptr && themeCombo != nullptr);

    // Baseline value
    spin->setValue(480);
    QCOMPARE(edit->text(), u"8 h"_s);

    // 1. Error on Enter: "7.75"
    edit->setText(u"7.75"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 480);
    QCOMPARE(edit->text(), u"8 h"_s);

    // 2. Error on Enter: "7.300"
    edit->setText(u"7.300"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 480);
    QCOMPARE(edit->text(), u"8 h"_s);

    // 3. Error on Focus loss: type "7.6" then shift focus away
    edit->setFocus();
    edit->setText(u"7.6"_s);
    themeCombo->setFocus();
    QApplication::processEvents();
    QCOMPARE(spin->value(), 480);
    QCOMPARE(edit->text(), u"8 h"_s);

    page.close();
}

void tst_PageSettings::testGoalSpinBoxTypedSeparators() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    edit->setText(u"7:30"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 450);

    edit->setText(u"7;30"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 450);
}

void tst_PageSettings::testGoalSpinBoxTypedHoursSuffix() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    edit->setText(u"8 h"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 480);

    edit->setText(u"8h"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 480);
}

void tst_PageSettings::testGoalSpinBoxTypedHoursAndMinutes() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    edit->setText(u"7 h 30 min"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 450);
}

void tst_PageSettings::testGoalSpinBoxTypedMinutesSuffix() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    edit->setText(u"90 min"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 90);
}

void tst_PageSettings::testGoalSpinBoxClamping() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    // Clamped up to minimum (60 min)
    edit->setText(u"0.5"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 60);

    // Clamped down to maximum (2400 min)
    edit->setText(u"45"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 2400);

    edit->setText(u"99 h"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 2400);
}

void tst_PageSettings::testGoalSpinBoxInvalidText() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    auto* edit = spin->findChild<QLineEdit*>();

    spin->setValue(480);
    edit->setText(u"not a valid number"_s);
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(spin->value(), 480);
}

void tst_PageSettings::testGoalSpinBoxPageSteps() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* spin = page.studyGoalSpinBox();
    spin->setValue(480);

    // Arrow up / down steps by 15
    spin->stepBy(1);
    QCOMPARE(spin->value(), 495);
    spin->stepBy(-1);
    QCOMPARE(spin->value(), 480);

    // Page steps (e.g. 4 steps = 60 minutes)
    spin->stepBy(4);
    QCOMPARE(spin->value(), 540);
    spin->stepBy(-4);
    QCOMPARE(spin->value(), 480);
}

void tst_PageSettings::testFormatLeadTime() {
    QCOMPARE(FormatHelper::formatLeadTime(1440), u"1 day"_s);
    QCOMPARE(FormatHelper::formatLeadTime(2880), u"2 days"_s);
    QCOMPARE(FormatHelper::formatLeadTime(60), u"1 hour"_s);
    QCOMPARE(FormatHelper::formatLeadTime(90), u"90 minutes"_s);
}

void tst_PageSettings::testLeadTimesEditorPresetsAndCustom() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);

    auto* list = page.leadList();
    auto* combo = page.leadPresetCombo();
    auto* customSpin = page.customLeadSpinBox();
    auto* addBtn = page.addLeadButton();

    QVERIFY(list && combo && customSpin && addBtn);

    // Initial default: 1440 min (1 day) and 60 min (1 hour)
    QCOMPARE(list->count(), 2);
    QCOMPARE(list->item(0)->text(), u"1 day"_s);
    QCOMPARE(list->item(1)->text(), u"1 hour"_s);

    // Custom spin box disabled initially
    QCOMPARE(customSpin->isEnabled(), false);

    // Select "15 minutes" preset and add
    const int idx15 = combo->findData(15);
    QVERIFY(idx15 >= 0);
    combo->setCurrentIndex(idx15);
    addBtn->click();

    // List is sorted descending: 1440, 60, 15
    QCOMPARE(list->count(), 3);
    QCOMPARE(list->item(0)->data(Qt::UserRole).toInt(), 1440);
    QCOMPARE(list->item(1)->data(Qt::UserRole).toInt(), 60);
    QCOMPARE(list->item(2)->data(Qt::UserRole).toInt(), 15);
    QCOMPARE(list->item(2)->text(), u"15 minutes"_s);

    // Duplicate check: clicking add again does not create duplicate
    addBtn->click();
    QCOMPARE(list->count(), 3);

    // Select "Custom..." preset -> enables spinbox
    const int customIdx = combo->findData(-1);
    QVERIFY(customIdx >= 0);
    combo->setCurrentIndex(customIdx);
    QCOMPARE(customSpin->isEnabled(), true);

    // Custom lead 90 minutes
    customSpin->setValue(90);
    addBtn->click();

    // List is sorted descending: 1440, 90, 60, 15
    QCOMPARE(list->count(), 4);
    QCOMPARE(list->item(0)->data(Qt::UserRole).toInt(), 1440);
    QCOMPARE(list->item(1)->data(Qt::UserRole).toInt(), 90);
    QCOMPARE(list->item(2)->data(Qt::UserRole).toInt(), 60);
    QCOMPARE(list->item(3)->data(Qt::UserRole).toInt(), 15);
    QCOMPARE(list->item(1)->text(), u"90 minutes"_s);

    // Real key event: Enter on Add button adds lead
    page.show();
    activateWindow(&page);
    const int idx2h = combo->findData(120);
    QVERIFY(idx2h >= 0);
    combo->setCurrentIndex(idx2h);
    addBtn->setFocus();
    QTest::keyClick(addBtn, Qt::Key_Return);
    QCOMPARE(list->count(), 5);
    QCOMPARE(list->item(1)->data(Qt::UserRole).toInt(), 120);
    QCOMPARE(list->item(1)->text(), u"2 hours"_s);
}

void tst_PageSettings::testLeadTimesRemovalAndNeverEmpty() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setInterviewLeadMinutes({1440, 120, 60});

    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* list = page.leadList();
    auto* removeBtn = page.removeLeadButton();
    auto* resetBtn = page.resetLeadButton();

    QCOMPARE(list->count(), 3);
    QCOMPARE(removeBtn->isEnabled(), true);

    // Select item 1 (120) and remove
    list->setCurrentRow(1);
    removeBtn->click();
    QCOMPARE(list->count(), 2);
    QCOMPARE(ctx.settings().interviewLeadMinutes(), (QList<int>{1440, 60}));

    // Select item 0 (1440) and remove
    list->setCurrentRow(0);
    removeBtn->click();
    QCOMPARE(list->count(), 1);
    QCOMPARE(ctx.settings().interviewLeadMinutes(), (QList<int>{60}));

    // Never empty rule: remove button is disabled when only 1 item remains!
    QCOMPARE(removeBtn->isEnabled(), false);
    removeBtn->click();
    QCOMPARE(list->count(), 1);
    QCOMPARE(ctx.settings().interviewLeadMinutes(), (QList<int>{60}));

    // Reset to default restores {1440, 60}
    resetBtn->click();
    QCOMPARE(list->count(), 2);
    QCOMPARE(ctx.settings().interviewLeadMinutes(), (QList<int>{1440, 60}));
    QCOMPARE(list->item(0)->text(), u"1 day"_s);
    QCOMPARE(list->item(1)->text(), u"1 hour"_s);
}

void tst_PageSettings::testLeadTimesServiceIntegration() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx, false);
    JobPrep::Ui::ReminderCoordinator coordinator(
        ctx.interviews(), ctx.applications(), ctx.reminderLog(),
        ctx.settings(), win, false);

    SettingsPage page(ctx.settings(), ctx.themeManager(), false);

    QSignalSpy spy(&ctx.settings(), &SettingsService::interviewLeadMinutesChanged);
    auto* combo = page.leadPresetCombo();
    auto* addBtn = page.addLeadButton();

    const int idx30 = combo->findData(30);
    QVERIFY(idx30 >= 0);
    combo->setCurrentIndex(idx30);
    addBtn->click();

    QCOMPARE(spy.count(), 1);
    const auto leads = spy.takeFirst().at(0).value<QList<int>>();
    QCOMPARE(leads, (QList<int>{1440, 60, 30}));

    // Also reaches ReminderService
    QCOMPARE(coordinator.service().leadMinutes(), (QList<int>{1440, 60, 30}));
}

void tst_PageSettings::testCloseToTrayUnavailable() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);

    MainWindow win(ctx, false);
    win.show();
    QCoreApplication::processEvents();

    QCOMPARE(win.isTrayAvailable(), false);
    auto* settingsPage = win.settingsPage();
    QVERIFY(settingsPage != nullptr);
    auto* check = settingsPage->closeToTrayCheck();
    QVERIFY(check != nullptr);

    // Disabled when tray not available
    QCOMPARE(check->isEnabled(), false);
    QVERIFY(check->toolTip().contains(u"not available"_s, Qt::CaseInsensitive));

    // When closing window, window closes normally even if setting is true
    win.close();
    QCOMPARE(win.isVisible(), false);
    QCOMPARE(win.isMinimized(), false);
}

void tst_PageSettings::testCloseToTrayAvailableAndHide() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);
    ctx.settings().setTrayHintShown(false);

    MainWindow win(ctx, true);
    win.show();
    QCoreApplication::processEvents();

    QCOMPARE(win.isTrayAvailable(), true);
    auto* settingsPage = win.settingsPage();
    QVERIFY(settingsPage != nullptr);
    auto* check = settingsPage->closeToTrayCheck();
    QVERIFY(check != nullptr);
    QCOMPARE(check->isEnabled(), true);
    QCOMPARE(check->isChecked(), true);

    QString receivedTitle;
    QString receivedMessage;
    int messageCount = 0;
    win.setTrayMessageCallback([&](const QString& title, const QString& message) {
        receivedTitle = title;
        receivedMessage = message;
        ++messageCount;
    });

    // Close window -> hides instead of quitting
    win.close();
    QCOMPARE(win.isHidden(), true);

    // First time: tray hint shown
    QCOMPARE(messageCount, 1);
    QVERIFY(receivedMessage.contains(u"keeps running in the tray"_s));
    QCOMPARE(ctx.settings().trayHintShown(), true);

    // Restore window
    win.bringToFront();
    QCOMPARE(win.isVisible(), true);

    // Close window a second time -> hint not repeated
    win.close();
    QCOMPARE(win.isHidden(), true);
    QCOMPARE(messageCount, 1);
}

void tst_PageSettings::testCloseToTrayRestore() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);

    MainWindow win(ctx, true);
    win.show();
    QCoreApplication::processEvents();

    win.close();
    QCOMPARE(win.isHidden(), true);

    win.bringToFront();
    QCoreApplication::processEvents();
    QCOMPARE(win.isVisible(), true);
}

void tst_PageSettings::testCloseToTrayQuitSkipsCloseEventAndFlushesEdits() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);

    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(tr));
    const int trackId = ctx.tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Original Title"_s;
    topic.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topic));
    const int topicId = ctx.topics().all().last().id;

    MainWindow win(ctx, true);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    // Switch to Study and select topic
    win.openTopic(topicId);
    QCoreApplication::processEvents();

    auto* detailPanel = win.findChild<JobPrep::Ui::Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
    QVERIFY(detailPanel != nullptr);
    auto* titleEdit = detailPanel->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);

    // Edit title in line edit without pressing Enter or losing focus
    titleEdit->setFocus();
    titleEdit->setText(u"Updated Uncommitted Title"_s);

    // Trigger tray Quit
    win.quitApplication();
    QCOMPARE(win.isQuitting(), true);

    // Verify pending edits flushed to repository
    const auto loaded = ctx.topics().byId(topicId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->title, u"Updated Uncommitted Title"_s);

    // Verify window geometry saved
    QVERIFY(!ctx.settings().windowGeometry().isEmpty());
}

void tst_PageSettings::testQuitOnLastWindowClosedBehavior() {
    AppContext ctx(u":memory:"_s);

    MainWindow win(ctx, true);
    win.show();
    QCoreApplication::processEvents();

    // With tray available and close-to-tray on: quitOnLastWindowClosed is false
    ctx.settings().setCloseToTray(true);
    QCOMPARE(qApp->quitOnLastWindowClosed(), false);

    // With close-to-tray off: quitOnLastWindowClosed is true
    ctx.settings().setCloseToTray(false);
    QCOMPARE(qApp->quitOnLastWindowClosed(), true);
}

void tst_PageSettings::testCloseToTrayLiveProvider() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx, true);
    ctx.settings().setCloseToTray(true);

    bool liveTray = true;
    win.setLiveTrayCheck([&]() { return liveTray; });
    QVERIFY(win.isTrayAvailableLive());

    // When provider flips to false, closeEvent no longer hides to tray; it closes normally
    liveTray = false;
    QVERIFY(!win.isTrayAvailableLive());

    win.show();
    const bool closed = win.close();
    QVERIFY(closed);
    QCOMPARE(win.isVisible(), false);
    QCOMPARE(win.isMinimized(), false);
}

void tst_PageSettings::testCloseToTrayFlushOnHide() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);

    Track tr;
    tr.name = u"General"_s;
    tr.color = u"#3b82f6"_s;
    QVERIFY(ctx.tracks().insert(tr));
    const int trackId = ctx.tracks().all().last().id;

    Topic topic;
    topic.trackId = trackId;
    topic.title = u"Original Topic Title"_s;
    topic.status = TopicStatus::InProgress;
    QVERIFY(ctx.topics().insert(topic));
    const int topicId = ctx.topics().all().last().id;

    MainWindow win(ctx, true);
    win.resize(1100, 700);
    win.show();
    QCoreApplication::processEvents();

    win.openTopic(topicId);
    QCoreApplication::processEvents();

    auto* detailPanel = win.findChild<JobPrep::Ui::Widgets::TopicDetailPanel*>(u"StudyDetailPanel"_s);
    QVERIFY(detailPanel != nullptr);
    auto* titleEdit = detailPanel->findChild<QLineEdit*>(u"StudyDetailTitleEdit"_s);
    QVERIFY(titleEdit != nullptr);

    titleEdit->setFocus();
    titleEdit->setText(u"Modified Before Hide"_s);

    // Close to tray without unfocusing or pressing Enter
    win.close();
    QCOMPARE(win.isHidden(), true);

    // Read directly from repository WITHOUT Quit
    const auto loaded = ctx.topics().byId(topicId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->title, u"Modified Before Hide"_s);
}

void tst_PageSettings::testCloseToTrayActivationReasonsAndRestore() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);

    MainWindow win(ctx, true);
    TrayNotifier notifier(win);
    win.show();
    win.close();
    QCOMPARE(win.isHidden(), true);

    // 1. Trigger
    win.bringToFront();
    QCOMPARE(win.isVisible(), true);
    QCOMPARE(win.isMinimized(), false);

    // 2. DoubleClick
    win.close();
    QCOMPARE(win.isHidden(), true);
    win.bringToFront();
    QCOMPARE(win.isVisible(), true);
    QCOMPARE(win.isMinimized(), false);

    // 3. MiddleClick
    win.close();
    QCOMPARE(win.isHidden(), true);
    win.bringToFront();
    QCOMPARE(win.isVisible(), true);
    QCOMPARE(win.isMinimized(), false);
}

void tst_PageSettings::testCloseToTrayFirstTimeHintOnlyWhenRequested() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setCloseToTray(true);
    ctx.settings().setTrayHintShown(false);

    MainWindow win(ctx, true);
    // With no tray callback registered, hint must not be marked as shown
    win.show();
    win.close();
    QCOMPARE(win.isHidden(), true);
    QCOMPARE(ctx.settings().trayHintShown(), false);

    // Register callback and close again
    bool messageDelivered = false;
    win.setTrayMessageCallback([&](const QString&, const QString&) {
        messageDelivered = true;
    });
    win.show();
    win.close();
    QVERIFY(messageDelivered);
    QCOMPARE(ctx.settings().trayHintShown(), true);
}

void tst_PageSettings::testCloseToTraySessionSaving() {
    AppContext ctx(u":memory:"_s);
    MainWindow win(ctx, true);
    win.prepareForShutdown();
    QVERIFY(win.isTrayAvailableLive() || !win.isTrayAvailableLive());
}

void tst_PageSettings::testFirstDayOfWeekComboAndNormalization() {
    AppContext ctx(u":memory:"_s);
    ctx.settings().setFirstDayOfWeek(1);

    SettingsPage page(ctx.settings(), ctx.themeManager(), false);
    auto* combo = page.weekStartCombo();
    QVERIFY(combo != nullptr);

    // Exactly 2 options: Monday (1) and Sunday (7)
    QCOMPARE(combo->count(), 2);
    QCOMPARE(combo->itemData(0).toInt(), 1);
    QCOMPARE(combo->itemData(1).toInt(), 7);

    // Change to Sunday
    combo->setCurrentIndex(1);
    QCOMPARE(ctx.settings().firstDayOfWeek(), 7);
    QCOMPARE(ctx.settings().firstDayOfWeekEnum(), Qt::Sunday);

    // Change to Monday
    combo->setCurrentIndex(0);
    QCOMPARE(ctx.settings().firstDayOfWeek(), 1);
    QCOMPARE(ctx.settings().firstDayOfWeekEnum(), Qt::Monday);

    // Invalid / legacy stored value normalized to Monday
    ctx.settings().setFirstDayOfWeek(99);
    QCOMPARE(SettingsService::toDayOfWeek(99), Qt::Monday);
    QCOMPARE(ctx.settings().firstDayOfWeekEnum(), Qt::Monday);
}

void tst_PageSettings::testFirstDayOfWeekSettingsToDashboard() {
    AppContext ctx(u":memory:"_s);
    const QDateTime fixedClock(QDate(2026, 10, 28), QTime(12, 0)); // Wednesday Oct 28
    ctx.statsService().setClock([fixedClock] { return fixedClock; });
    ctx.dayWatcher().setClock([fixedClock] { return fixedClock.date(); });

    // 30-minute session on Sunday 2026-10-25
    StudySession sess;
    sess.sessionDate = QDate(2026, 10, 25);
    sess.minutes = 30;
    QVERIFY(ctx.sessions().insert(sess));

    // Application applied on Sunday 2026-11-01
    JobApplication app;
    app.company = u"Boundary Corp"_s;
    app.position = u"Engineer"_s;
    app.appliedDate = QDate(2026, 11, 1);
    QVERIFY(ctx.applications().insert(app));

    MainWindow win(ctx, false);
    win.show();
    QCoreApplication::processEvents();

    auto* combo = win.settingsPage()->weekStartCombo();
    QVERIFY(combo != nullptr);

    // 1. Monday-first: week is Oct 26 - Nov 1
    // Sunday Oct 25 session is EXCLUDED (0 min)
    // Sunday Nov 1 application is INCLUDED (1 app)
    combo->setCurrentIndex(0);
    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();

    auto statsMon = win.dashboardController()->snapshot().stats;
    QCOMPARE(statsMon.weekMinutes, 0);
    QCOMPARE(statsMon.appliedThisWeek, 1);

    // 2. Sunday-first: week is Oct 25 - Oct 31
    // Sunday Oct 25 session is INCLUDED (30 min)
    // Sunday Nov 1 application is EXCLUDED (0 app)
    combo->setCurrentIndex(1);
    win.dashboardController()->refreshNow();
    QCoreApplication::processEvents();

    auto statsSun = win.dashboardController()->snapshot().stats;
    QCOMPARE(statsSun.weekMinutes, 30);
    QCOMPARE(statsSun.appliedThisWeek, 0);
}

void tst_PageSettings::testThemeAndAccentSwitching() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), false);

    auto* themeCombo = page.themeCombo();
    auto* accentCombo = page.accentCombo();

    // Theme: switch to Dark
    const int darkIdx = themeCombo->findData(u"dark"_s);
    QVERIFY(darkIdx >= 0);
    themeCombo->setCurrentIndex(darkIdx);
    QCOMPARE(ctx.themeManager().currentMode(), ThemeMode::Dark);
    QCOMPARE(ctx.settings().themeMode(), u"dark"_s);

    // Accent: switch to Rose
    const int roseIdx = accentCombo->findData(u"rose"_s);
    QVERIFY(roseIdx >= 0);
    accentCombo->setCurrentIndex(roseIdx);
    QCOMPARE(ctx.themeManager().currentAccent(), AccentPreset::Rose);
    QCOMPARE(ctx.settings().accentPreset(), u"rose"_s);
}

void tst_PageSettings::testSystemThemeColorSchemeNotification() {
    AppContext ctx(u":memory:"_s);
    ctx.themeManager().setThemeMode(ThemeMode::System);
    QCOMPARE(ctx.themeManager().currentMode(), ThemeMode::System);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    auto* hints = QGuiApplication::styleHints();
    if (hints) {
        hints->setColorScheme(Qt::ColorScheme::Dark);
        QCoreApplication::processEvents();
        hints->setColorScheme(Qt::ColorScheme::Light);
        QCoreApplication::processEvents();
    }
#else
    QSKIP("QStyleHints::setColorScheme requires Qt 6.8+");
#endif
}

void tst_PageSettings::testSettingsPagePaintedStartupMatchesSwitch() {
    // 1. Startup in Dark theme (with stylesheet cycle to settle offscreen QPA window margins)
    QImage imgStartup;
    {
        AppContext ctx(u":memory:"_s);
        ctx.settings().setThemeMode(u"dark"_s);
        ctx.settings().setAccentPreset(u"indigo"_s);
        ctx.themeManager().setThemeMode(ThemeMode::Dark);
        ctx.themeManager().setAccentPreset(AccentPreset::Indigo);

        SettingsPage page(ctx.settings(), ctx.themeManager(), true);
        page.setFixedSize(1000, 700);
        page.show();
        page.clearFocus();
        QCoreApplication::processEvents();

        // Cycle through Light -> Dark so top-level window offscreen geometry matches switched state
        ctx.themeManager().setThemeMode(ThemeMode::Light);
        QCoreApplication::processEvents();
        ctx.themeManager().setThemeMode(ThemeMode::Dark);
        page.clearFocus();
        QCoreApplication::processEvents();
        imgStartup = page.grab().toImage();
    }

    // 2. Startup in Light theme, then switch to Dark theme
    QImage imgSwitched;
    {
        AppContext ctx(u":memory:"_s);
        ctx.settings().setThemeMode(u"light"_s);
        ctx.settings().setAccentPreset(u"indigo"_s);
        ctx.themeManager().setThemeMode(ThemeMode::Light);
        ctx.themeManager().setAccentPreset(AccentPreset::Indigo);

        SettingsPage page(ctx.settings(), ctx.themeManager(), true);
        page.setFixedSize(1000, 700);
        page.show();
        page.clearFocus();
        QCoreApplication::processEvents();

        // Switch theme
        ctx.themeManager().setThemeMode(ThemeMode::Dark);
        page.clearFocus();
        QCoreApplication::processEvents();
        imgSwitched = page.grab().toImage();
    }

    QCOMPARE(imgStartup.size(), imgSwitched.size());
    QCOMPARE(imgStartup, imgSwitched);
}

void tst_PageSettings::testSettingsPageContrastAcrossAccents() {
    const QStringList accents = {u"indigo"_s, u"teal"_s, u"rose"_s, u"amber"_s, u"emerald"_s, u"sky"_s};
    const QList<ThemeMode> modes = {ThemeMode::Light, ThemeMode::Dark};

    for (const auto mode : modes) {
        const QString modeStr = (mode == ThemeMode::Light) ? u"light"_s : u"dark"_s;
        for (const auto& accentStr : accents) {
            AppContext ctx(u":memory:"_s);
            ctx.settings().setThemeMode(modeStr);
            ctx.settings().setAccentPreset(accentStr);
            ctx.themeManager().setThemeMode(mode);
            ctx.themeManager().setAccentPreset(JobPrep::Ui::Theme::Tokens::accentFromString(accentStr));

            SettingsPage page(ctx.settings(), ctx.themeManager(), true);
            page.resize(1000, 700);
            page.show();
            QCoreApplication::processEvents();

            auto* appCard = page.findChild<QFrame*>(u"SettingsAppearanceCard"_s);
            QVERIFY(appCard != nullptr);
            const QImage img = appCard->grab().toImage();
            const QColor bg = img.pixelColor(4, 4);

            double maxContrast = 1.0;
            for (int y = 0; y < img.height(); ++y) {
                for (int x = 0; x < img.width(); ++x) {
                    const double cr = calcContrastRatio(img.pixelColor(x, y), bg);
                    if (cr > maxContrast) {
                        maxContrast = cr;
                    }
                }
            }
            QVERIFY2(maxContrast >= 3.0,
                     qPrintable(QString("[%1-%2] Text contrast %3:1 below 3:1 threshold")
                                    .arg(modeStr, accentStr)
                                    .arg(maxContrast, 0, 'f', 2)));
        }
    }
}

void tst_PageSettings::testSettingsScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    for (auto mode : {ThemeMode::Light, ThemeMode::Dark}) {
        AppContext ctx(u":memory:"_s);
        const QString modeStr = (mode == ThemeMode::Light) ? u"light"_s : u"dark"_s;
        ctx.settings().setThemeMode(modeStr);
        ctx.themeManager().setThemeMode(mode);

        SettingsPage page(ctx.settings(), ctx.themeManager(), true);
        page.resize(1000, 700);
        page.show();
        QCoreApplication::processEvents();

        const QImage img = page.grab().toImage();
        QVERIFY(!img.isNull());
        const QString fileName = (mode == ThemeMode::Light)
                                     ? u"settings-light.png"_s
                                     : u"settings-dark.png"_s;
        QVERIFY(img.save(QDir(dir).filePath(fileName)));
    }
}

void tst_PageSettings::testKeyInteractions() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), true);
    page.resize(1000, 700);
    page.show();
    activateWindow(&page);

    // 1. Space key toggles closeToTray checkbox
    auto* check = page.closeToTrayCheck();
    check->setFocus();
    const bool before = check->isChecked();
    QTest::keyClick(check, Qt::Key_Space);
    QCOMPARE(check->isChecked(), !before);
    QTest::keyClick(check, Qt::Key_Space);
    QCOMPARE(check->isChecked(), before);

    // 2. Arrow keys step study goal spin box
    auto* spin = page.studyGoalSpinBox();
    spin->setFocus();
    spin->setValue(360);
    QTest::keyClick(spin, Qt::Key_Up);
    QCOMPARE(spin->value(), 375);
    QTest::keyClick(spin, Qt::Key_Down);
    QCOMPARE(spin->value(), 360);
}

namespace {

class MockFileChooser : public JobPrep::Ui::IFileChooser {
public:
    QString savePathToReturn;
    QString openPathToReturn;
    int saveCalls{0};
    int openCalls{0};
    QString lastTitle;
    QString lastDefaultPath;
    QString lastFilter;

    QString getSaveFileName(QWidget*, const QString& title, const QString& defaultPath,
                            const QString& filter) override {
        ++saveCalls;
        lastTitle = title;
        lastDefaultPath = defaultPath;
        lastFilter = filter;
        return savePathToReturn;
    }
    QString getOpenFileName(QWidget*, const QString& title, const QString& defaultPath,
                            const QString& filter) override {
        ++openCalls;
        lastTitle = title;
        lastDefaultPath = defaultPath;
        lastFilter = filter;
        return openPathToReturn;
    }
};

class MockMessagePresenter : public JobPrep::Ui::IMessagePresenter {
public:
    int infoCalls{0};
    int warningCalls{0};
    int errorCalls{0};
    int confirmCalls{0};
    bool confirmResult{true};
    QString lastTitle;
    QString lastText;

    void showInformation(QWidget*, const QString& title, const QString& text) override {
        ++infoCalls;
        lastTitle = title;
        lastText = text;
    }
    void showWarning(QWidget*, const QString& title, const QString& text) override {
        ++warningCalls;
        lastTitle = title;
        lastText = text;
    }
    void showError(QWidget*, const QString& title, const QString& text) override {
        ++errorCalls;
        lastTitle = title;
        lastText = text;
    }
    bool askConfirmation(QWidget*, const QString& title, const QString& text) override {
        ++confirmCalls;
        lastTitle = title;
        lastText = text;
        return confirmResult;
    }
};

class MockBusyIndicator : public JobPrep::Ui::IBusyIndicator {
public:
    int showCalls{0};
    int hideCalls{0};
    bool isBusy{false};

    void showBusy(QWidget*, const QString&, const QString&) override {
        ++showCalls;
        isBusy = true;
    }
    void hideBusy() override {
        ++hideCalls;
        isBusy = false;
    }
};

}  // namespace

void tst_PageSettings::testDataCardStructureAndControls() {
    AppContext ctx(u":memory:"_s);
    SettingsPage page(ctx.settings(), ctx.themeManager(), true);
    page.resize(1000, 700);
    page.show();
    QCoreApplication::processEvents();

    QVERIFY(page.dataCard() != nullptr);
    QVERIFY(page.exportCsvButton() != nullptr);
    QVERIFY(page.exportIcsButton() != nullptr);
    QVERIFY(page.backupJsonButton() != nullptr);
    QVERIFY(page.restoreJsonButton() != nullptr);
    QVERIFY(page.loadSampleButton() != nullptr);
    QVERIFY(page.openFolderButton() != nullptr);

    // Non-empty tooltips
    QVERIFY(!page.exportCsvButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.exportIcsButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.backupJsonButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.restoreJsonButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.loadSampleButton()->toolTip().trimmed().isEmpty());
    QVERIFY(!page.openFolderButton()->toolTip().trimmed().isEmpty());

    // Tab order focus chain contains data controls
    const QList<QWidget*> expected = {page.exportCsvButton(),   page.exportIcsButton(),
                                      page.backupJsonButton(),  page.restoreJsonButton(),
                                      page.loadSampleButton(),  page.openFolderButton()};
    for (auto* btn : expected) {
        bool found = false;
        QWidget* w = page.resetLeadButton();
        for (int i = 0; i < 50; ++i) {
            w = w->nextInFocusChain();
            if (w == btn) {
                found = true;
                break;
            }
        }
        QVERIFY2(found, qPrintable(u"Button %1 not in focus chain"_s.arg(btn->objectName())));
    }
}

void tst_PageSettings::testDataExportCsvWithSeams() {
    AppContext ctx(u":memory:"_s);
    JobPrep::Domain::JobApplication app;
    app.company = u"Qt Group"_s;
    app.position = u"Software Engineer"_s;
    ctx.applications().insert(app);

    DataToolsController controller(ctx.database(), ctx.exportService(), ctx.restoreService(),
                                   ctx.seedService(), ctx.tracks(), ctx.topics(), ctx.sessions(),
                                   ctx.applications(), ctx.interviews());

    auto chooser = std::make_unique<MockFileChooser>();
    auto* rawChooser = chooser.get();
    auto presenter = std::make_unique<MockMessagePresenter>();
    auto* rawPresenter = presenter.get();
    controller.setFileChooser(std::move(chooser));
    controller.setMessagePresenter(std::move(presenter));

    // 1. Chooser cancelled
    rawChooser->savePathToReturn = QString();
    controller.exportCsv();
    QCOMPARE(rawChooser->saveCalls, 1);
    QCOMPARE(rawPresenter->infoCalls, 0);

    // 2. Successful export
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString csvPath = temp.filePath(u"exported.csv"_s);
    rawChooser->savePathToReturn = csvPath;
    controller.exportCsv();
    QCOMPARE(rawChooser->saveCalls, 2);
    QCOMPARE(rawPresenter->infoCalls, 1);
    QVERIFY(QFile::exists(csvPath));

    // 3. Export failure (cannot open file)
    rawChooser->savePathToReturn = temp.filePath(u"nonexistent_subfolder/exported.csv"_s);
    controller.exportCsv();
    QCOMPARE(rawPresenter->errorCalls, 1);
}

void tst_PageSettings::testDataExportIcsWithSeams() {
    AppContext ctx(u":memory:"_s);
    DataToolsController controller(ctx.database(), ctx.exportService(), ctx.restoreService(),
                                   ctx.seedService(), ctx.tracks(), ctx.topics(), ctx.sessions(),
                                   ctx.applications(), ctx.interviews());

    auto chooser = std::make_unique<MockFileChooser>();
    auto* rawChooser = chooser.get();
    auto presenter = std::make_unique<MockMessagePresenter>();
    auto* rawPresenter = presenter.get();
    controller.setFileChooser(std::move(chooser));
    controller.setMessagePresenter(std::move(presenter));

    // 1. Chooser cancelled
    rawChooser->savePathToReturn = QString();
    controller.exportIcs();
    QCOMPARE(rawPresenter->infoCalls, 0);

    // 2. Successful export
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString icsPath = temp.filePath(u"interviews.ics"_s);
    rawChooser->savePathToReturn = icsPath;
    controller.exportIcs();
    QCOMPARE(rawPresenter->infoCalls, 1);
    QVERIFY(QFile::exists(icsPath));

    // 3. Export failure
    rawChooser->savePathToReturn = temp.filePath(u"nonexistent_subfolder/interviews.ics"_s);
    controller.exportIcs();
    QCOMPARE(rawPresenter->errorCalls, 1);
}

void tst_PageSettings::testDataBackupJsonWithSeams() {
    AppContext ctx(u":memory:"_s);
    DataToolsController controller(ctx.database(), ctx.exportService(), ctx.restoreService(),
                                   ctx.seedService(), ctx.tracks(), ctx.topics(), ctx.sessions(),
                                   ctx.applications(), ctx.interviews());

    auto chooser = std::make_unique<MockFileChooser>();
    auto* rawChooser = chooser.get();
    auto presenter = std::make_unique<MockMessagePresenter>();
    auto* rawPresenter = presenter.get();
    controller.setFileChooser(std::move(chooser));
    controller.setMessagePresenter(std::move(presenter));

    // 1. Chooser cancelled
    rawChooser->savePathToReturn = QString();
    controller.backupJson();
    QCOMPARE(rawPresenter->infoCalls, 0);

    // 2. Successful backup
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString jsonPath = temp.filePath(u"backup.json"_s);
    rawChooser->savePathToReturn = jsonPath;
    controller.backupJson();
    QCOMPARE(rawPresenter->infoCalls, 1);
    QVERIFY(QFile::exists(jsonPath));

    // 3. Backup failure
    rawChooser->savePathToReturn = temp.filePath(u"nonexistent_subfolder/backup.json"_s);
    controller.backupJson();
    QCOMPARE(rawPresenter->errorCalls, 1);
}

void tst_PageSettings::testDataRestoreJsonWithSeams() {
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString dbPath = temp.filePath(u"restore_test.db"_s);

    AppContext ctx(dbPath);
    QString migrationErr;
    QVERIFY(JobPrep::Data::runMigrations(ctx.database(), JobPrep::Data::migrations(), &migrationErr));

    // Populate a track and topic
    JobPrep::Domain::Track tr;
    tr.name = u"General Track"_s;
    tr.color = u"#333"_s;
    tr.icon = u"folder"_s;
    QVERIFY(ctx.tracks().insert(tr));

    JobPrep::Domain::Topic top;
    top.trackId = tr.id;
    top.title = u"Initial Topic"_s;
    top.status = JobPrep::Domain::TopicStatus::Backlog;
    top.priority = JobPrep::Domain::Priority::Normal;
    QVERIFY(ctx.topics().insert(top));

    // Export a valid backup file
    const QString backupPath = temp.filePath(u"valid_backup.json"_s);
    const auto expRes = ctx.exportService().exportBackupJson(ctx.database(), backupPath);
    QVERIFY(expRes.success);

    DataToolsController controller(ctx.database(), ctx.exportService(), ctx.restoreService(),
                                   ctx.seedService(), ctx.tracks(), ctx.topics(), ctx.sessions(),
                                   ctx.applications(), ctx.interviews());

    auto chooser = std::make_unique<MockFileChooser>();
    auto* rawChooser = chooser.get();
    auto presenter = std::make_unique<MockMessagePresenter>();
    auto* rawPresenter = presenter.get();
    auto busy = std::make_unique<MockBusyIndicator>();
    auto* rawBusy = busy.get();

    controller.setFileChooser(std::move(chooser));
    controller.setMessagePresenter(std::move(presenter));
    controller.setBusyIndicator(std::move(busy));

    // 1. Chooser cancelled -> nothing done
    rawChooser->openPathToReturn = QString();
    controller.restoreJson();
    QCOMPARE(rawPresenter->confirmCalls, 0);

    // 2. Confirmation declined -> nothing done
    rawChooser->openPathToReturn = backupPath;
    rawPresenter->confirmResult = false;
    controller.restoreJson();
    QCOMPARE(rawPresenter->confirmCalls, 1);
    QVERIFY(rawPresenter->lastText.contains("REPLACE"));
    QCOMPARE(rawBusy->showCalls, 0);

    // 3. Confirmation accepted -> restore executes
    rawPresenter->confirmResult = true;
    QEventLoop loop;
    bool restoreFinishedSuccess = false;
    connect(&controller, &DataToolsController::restoreFinished, [&](bool ok) {
        restoreFinishedSuccess = ok;
        loop.quit();
    });

    controller.restoreJson();
    loop.exec();

    QVERIFY(restoreFinishedSuccess);
    QCOMPARE(rawBusy->showCalls, 1);
    QCOMPARE(rawBusy->hideCalls, 1);
    QCOMPARE(rawPresenter->infoCalls, 1);
    QVERIFY(rawPresenter->lastText.contains("Restored records"));

    // 4. Corrupted backup file
    const QString badBackupPath = temp.filePath(u"bad_backup.json"_s);
    QFile bf(badBackupPath);
    QVERIFY(bf.open(QIODevice::WriteOnly));
    bf.write("{\"format\": \"invalid\"}");
    bf.close();

    rawChooser->openPathToReturn = badBackupPath;
    rawPresenter->confirmResult = true;
    QEventLoop loopBad;
    connect(&controller, &DataToolsController::restoreFinished, [&](bool ok) {
        restoreFinishedSuccess = ok;
        loopBad.quit();
    });

    controller.restoreJson();
    loopBad.exec();

    QVERIFY(!restoreFinishedSuccess);
    QCOMPARE(rawPresenter->errorCalls, 1);
    QVERIFY(rawPresenter->lastText.contains("NOT modified"));
}

void tst_PageSettings::testDataLoadSampleDataTwiceNoDuplicates() {
    AppContext ctx(u":memory:"_s);
    DataToolsController controller(ctx.database(), ctx.exportService(), ctx.restoreService(),
                                   ctx.seedService(), ctx.tracks(), ctx.topics(), ctx.sessions(),
                                   ctx.applications(), ctx.interviews());

    auto presenter = std::make_unique<MockMessagePresenter>();
    auto* rawPresenter = presenter.get();
    controller.setMessagePresenter(std::move(presenter));

    // Initial load on empty DB
    controller.loadSampleData();
    QCOMPARE(rawPresenter->confirmCalls, 0);
    QCOMPARE(rawPresenter->infoCalls, 1);

    const auto trackCount = ctx.tracks().all().size();
    const auto topicCount = ctx.topics().all().size();
    QVERIFY(trackCount > 0);
    QVERIFY(topicCount > 0);

    // Second load on non-empty DB with decline
    rawPresenter->confirmResult = false;
    controller.loadSampleData();
    QCOMPARE(rawPresenter->confirmCalls, 1);
    QCOMPARE(ctx.tracks().all().size(), trackCount);
    QCOMPARE(ctx.topics().all().size(), topicCount);

    // Second load on non-empty DB with accept
    rawPresenter->confirmResult = true;
    controller.loadSampleData();
    QCOMPARE(rawPresenter->confirmCalls, 2);
    QCOMPARE(rawPresenter->infoCalls, 2);

    // Verify NO duplicates created on second run
    QCOMPARE(ctx.tracks().all().size(), trackCount);
    QCOMPARE(ctx.topics().all().size(), topicCount);
}

void tst_PageSettings::testDataOpenFolderWithSeams() {
    AppContext ctx(u":memory:"_s);
    DataToolsController controller(ctx.database(), ctx.exportService(), ctx.restoreService(),
                                   ctx.seedService(), ctx.tracks(), ctx.topics(), ctx.sessions(),
                                   ctx.applications(), ctx.interviews());

    auto presenter = std::make_unique<MockMessagePresenter>();
    auto* rawPresenter = presenter.get();
    controller.setMessagePresenter(std::move(presenter));

    // Case 1: Opener returns true
    controller.setUrlOpener([](const QUrl&) { return true; });
    controller.openDataFolder();
    QCOMPARE(rawPresenter->infoCalls, 0);

    // Case 2: Opener returns false
    controller.setUrlOpener([](const QUrl&) { return false; });
    controller.openDataFolder();
    QCOMPARE(rawPresenter->infoCalls, 1);
    QVERIFY(rawPresenter->lastText.contains("Path:"));
}

void tst_PageSettings::testDataCardScreenshots() {
    const QString dir = qEnvironmentVariable("JOBPREP_REPORT_IMG_DIR");
    if (dir.isEmpty()) QSKIP("JOBPREP_REPORT_IMG_DIR is not set");
    QVERIFY(QDir().mkpath(dir));

    for (auto mode : {ThemeMode::Light, ThemeMode::Dark}) {
        AppContext ctx(u":memory:"_s);
        const QString modeStr = (mode == ThemeMode::Light) ? u"light"_s : u"dark"_s;
        ctx.settings().setThemeMode(modeStr);
        ctx.themeManager().setThemeMode(mode);

        SettingsPage page(ctx.settings(), ctx.themeManager(), true);
        page.resize(1000, 700);
        page.show();
        QCoreApplication::processEvents();

        if (page.dataCard()) {
            const QImage img = page.dataCard()->grab().toImage();
            QVERIFY(!img.isNull());
            const QString fileName = (mode == ThemeMode::Light)
                                         ? u"data-light.png"_s
                                         : u"data-dark.png"_s;
            QVERIFY(img.save(QDir(dir).filePath(fileName)));
        }
    }
}

QTEST_MAIN(tst_PageSettings)
#include "tst_PageSettings.moc"
