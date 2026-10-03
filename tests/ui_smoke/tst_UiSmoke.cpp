#include <QApplication>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStringList>
#include <QTest>
#include "app/AppContext.h"
#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace {

QStringList s_capturedWarnings;

void testMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        s_capturedWarnings.append(msg);
    }
}

}  // namespace

class tst_UiSmoke : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testWindowCreationAndNavigation();
    void testThemeAndAccentSwitching();
    void testSidebarCollapsing();
    void testZeroWarnings();
    void cleanupTestCase();

private:
    std::unique_ptr<JobPrep::App::AppContext> m_ctx;
    std::unique_ptr<JobPrep::Ui::MainWindow> m_window;
};

void tst_UiSmoke::initTestCase() {
    qInstallMessageHandler(testMessageHandler);

    QCoreApplication::setOrganizationName(u"JobPrepTest"_s);
    QCoreApplication::setApplicationName(u"JobPrepTest"_s);

    m_ctx = std::make_unique<JobPrep::App::AppContext>();
    m_window = std::make_unique<JobPrep::Ui::MainWindow>(*m_ctx);
    m_window->show();
}

void tst_UiSmoke::testWindowCreationAndNavigation() {
    QVERIFY(m_window != nullptr);
    QVERIFY(m_window->pageStack() != nullptr);
    QCOMPARE(m_window->pageStack()->count(), 5);

    // Initial page is Dashboard (0)
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);

    // Walk through all pages
    for (int i = 0; i < 5; ++i) {
        m_window->sidebar()->setCurrentPage(i);
        emit m_window->sidebar()->pageSelected(i);
        QCOMPARE(m_window->pageStack()->currentIndex(), i);
        QVERIFY(m_window->pageStack()->currentWidget() != nullptr);
    }

    // Return to first page
    m_window->sidebar()->setCurrentPage(0);
    emit m_window->sidebar()->pageSelected(0);
    QCOMPARE(m_window->pageStack()->currentIndex(), 0);
}

void tst_UiSmoke::testThemeAndAccentSwitching() {
    auto& themeMgr = m_ctx->themeManager();

    // Toggle Light mode
    themeMgr.setThemeMode(JobPrep::Ui::Theme::ThemeMode::Light);
    QCOMPARE(themeMgr.currentMode(), JobPrep::Ui::Theme::ThemeMode::Light);
    QVERIFY(!themeMgr.isDark());

    // Toggle Dark mode
    themeMgr.setThemeMode(JobPrep::Ui::Theme::ThemeMode::Dark);
    QCOMPARE(themeMgr.currentMode(), JobPrep::Ui::Theme::ThemeMode::Dark);
    QVERIFY(themeMgr.isDark());

    // Cycle through all 6 accent presets
    const JobPrep::Ui::Theme::AccentPreset accents[] = {
        JobPrep::Ui::Theme::AccentPreset::Indigo,
        JobPrep::Ui::Theme::AccentPreset::Teal,
        JobPrep::Ui::Theme::AccentPreset::Rose,
        JobPrep::Ui::Theme::AccentPreset::Amber,
        JobPrep::Ui::Theme::AccentPreset::Emerald,
        JobPrep::Ui::Theme::AccentPreset::Sky,
    };

    for (auto accent : accents) {
        themeMgr.setAccentPreset(accent);
        QCOMPARE(themeMgr.currentAccent(), accent);
        const auto tokens = themeMgr.currentTokens();
        QVERIFY(tokens.accent.isValid());
        QVERIFY(tokens.bg.isValid());
        QVERIFY(tokens.surface.isValid());
    }
}

void tst_UiSmoke::testSidebarCollapsing() {
    auto* sidebar = m_window->sidebar();
    QVERIFY(sidebar != nullptr);

    // Test collapse
    sidebar->setCollapsed(true);
    QVERIFY(sidebar->isCollapsed());
    QCOMPARE(sidebar->width(), JobPrep::Ui::Sidebar::kWidthCollapsed);

    // Test expand
    sidebar->setCollapsed(false);
    QVERIFY(!sidebar->isCollapsed());
    QCOMPARE(sidebar->width(), JobPrep::Ui::Sidebar::kWidthExpanded);
}

void tst_UiSmoke::testZeroWarnings() {
    if (!s_capturedWarnings.isEmpty()) {
        const QString failures = s_capturedWarnings.join(u"\n"_s);
        QFAIL(qPrintable(u"Captured unexpected Qt warnings/criticals:\n"_s + failures));
    }
    QVERIFY(s_capturedWarnings.isEmpty());
}

void tst_UiSmoke::cleanupTestCase() {
    m_window.reset();
    m_ctx.reset();
    qInstallMessageHandler(nullptr);
}

QTEST_MAIN(tst_UiSmoke)
#include "tst_UiSmoke.moc"
