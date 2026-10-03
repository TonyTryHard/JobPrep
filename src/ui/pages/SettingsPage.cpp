#include "ui/pages/SettingsPage.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include "services/SettingsService.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

SettingsPage::SettingsPage(Services::SettingsService& settings,
                           Theme::ThemeManager& themeMgr,
                           QWidget* parent)
    : PageBase(tr("Settings"), parent), m_settings(settings), m_themeMgr(themeMgr) {
    setupUi();
    loadSettings();
}

void SettingsPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget(scrollArea);
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(24);

    // Appearance Card
    auto* appearanceCard = new QFrame(container);
    appearanceCard->setProperty("card", true);
    auto* appearanceLayout = new QVBoxLayout(appearanceCard);
    appearanceLayout->setContentsMargins(24, 24, 24, 24);
    appearanceLayout->setSpacing(16);

    auto* appTitle = new QLabel(tr("Appearance"), appearanceCard);
    appTitle->setStyleSheet(u"font-size: 12pt; font-weight: 600;"_s);
    appearanceLayout->addWidget(appTitle);

    auto* appForm = new QFormLayout();
    appForm->setSpacing(12);

    m_themeCombo = new QComboBox(appearanceCard);
    m_themeCombo->addItem(tr("System Default"), u"system"_s);
    m_themeCombo->addItem(tr("Light"), u"light"_s);
    m_themeCombo->addItem(tr("Dark"), u"dark"_s);
    appForm->addRow(tr("Theme:"), m_themeCombo);

    m_accentCombo = new QComboBox(appearanceCard);
    m_accentCombo->addItem(tr("Indigo"), u"indigo"_s);
    m_accentCombo->addItem(tr("Teal"), u"teal"_s);
    m_accentCombo->addItem(tr("Rose"), u"rose"_s);
    m_accentCombo->addItem(tr("Amber"), u"amber"_s);
    m_accentCombo->addItem(tr("Emerald"), u"emerald"_s);
    m_accentCombo->addItem(tr("Sky"), u"sky"_s);
    appForm->addRow(tr("Accent color:"), m_accentCombo);

    appearanceLayout->addLayout(appForm);
    containerLayout->addWidget(appearanceCard);

    // Study & Calendar Preferences Card
    auto* prefsCard = new QFrame(container);
    prefsCard->setProperty("card", true);
    auto* prefsLayout = new QVBoxLayout(prefsCard);
    prefsLayout->setContentsMargins(24, 24, 24, 24);
    prefsLayout->setSpacing(16);

    auto* prefsTitle = new QLabel(tr("Preferences"), prefsCard);
    prefsTitle->setStyleSheet(u"font-size: 12pt; font-weight: 600;"_s);
    prefsLayout->addWidget(prefsTitle);

    auto* prefsForm = new QFormLayout();
    prefsForm->setSpacing(12);

    m_studyGoalSpinBox = new QSpinBox(prefsCard);
    m_studyGoalSpinBox->setRange(1, 40);
    m_studyGoalSpinBox->setSuffix(tr(" hours / week"));
    prefsForm->addRow(tr("Weekly study goal:"), m_studyGoalSpinBox);

    m_weekStartCombo = new QComboBox(prefsCard);
    m_weekStartCombo->addItem(tr("Monday"), 1);
    m_weekStartCombo->addItem(tr("Sunday"), 7);
    prefsForm->addRow(tr("First day of week:"), m_weekStartCombo);

    m_closeToTrayCheck = new QCheckBox(tr("Minimize / Close to system tray"), prefsCard);
    prefsForm->addRow(QString(), m_closeToTrayCheck);

    prefsLayout->addLayout(prefsForm);
    containerLayout->addWidget(prefsCard);

    containerLayout->addStretch(1);
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);

    // Wire controls
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString modeStr = m_themeCombo->itemData(index).toString();
        m_themeMgr.setThemeMode(Theme::Tokens::themeModeFromString(modeStr));
    });

    connect(m_accentCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString accentStr = m_accentCombo->itemData(index).toString();
        m_themeMgr.setAccentPreset(Theme::Tokens::accentFromString(accentStr));
    });

    connect(m_studyGoalSpinBox, &QSpinBox::valueChanged, this, [this](int hours) {
        m_settings.setWeeklyGoalMinutes(hours * 60);
    });

    connect(m_weekStartCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const int day = m_weekStartCombo->itemData(index).toInt();
        m_settings.setFirstDayOfWeek(day);
    });

    connect(m_closeToTrayCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setCloseToTray(checked);
    });
}

void SettingsPage::loadSettings() {
    // Theme mode
    const QString themeMode = m_settings.themeMode();
    const int themeIdx = m_themeCombo->findData(themeMode);
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    // Accent preset
    const QString accent = m_settings.accentPreset();
    const int accentIdx = m_accentCombo->findData(accent);
    if (accentIdx >= 0) m_accentCombo->setCurrentIndex(accentIdx);

    // Weekly study goal (minutes to hours)
    const int hours = qMax(1, m_settings.weeklyGoalMinutes() / 60);
    m_studyGoalSpinBox->setValue(hours);

    // First day of week
    const int day = m_settings.firstDayOfWeek();
    const int dayIdx = m_weekStartCombo->findData(day);
    if (dayIdx >= 0) m_weekStartCombo->setCurrentIndex(dayIdx);

    // Close to tray
    m_closeToTrayCheck->setChecked(m_settings.closeToTray());
}

}  // namespace JobPrep::Ui::Pages
