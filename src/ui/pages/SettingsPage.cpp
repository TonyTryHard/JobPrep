#include "ui/pages/SettingsPage.h"

#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include "services/SettingsService.h"
#include "ui/DataToolsController.h"
#include "ui/FormatHelper.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Pages {

namespace {

std::optional<int> parseGoalMinutes(const QString& text) {
    const QString clean = text.trimmed();
    if (clean.isEmpty()) {
        return std::nullopt;
    }

    // 1. Explicit minutes with 'min' suffix: e.g. "90 min", "90min", "90 mins", "90 minute", "90 minutes"
    static const QRegularExpression minOnlyRe(
        uR"(^(\d+)\s*(?:min|mins|minutes?)$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    auto match = minOnlyRe.match(clean);
    if (match.hasMatch()) {
        bool ok = false;
        const int mins = match.captured(1).toInt(&ok);
        if (!ok) return std::nullopt;
        const int rounded = static_cast<int>(std::round(static_cast<double>(mins) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    // 2. Hours and minutes with 'h': e.g. "7 h 30 min", "7h 30 min", "7h 30", "7 h 30"
    static const QRegularExpression hMinRe(
        uR"(^(\d+)\s*h\s*(\d{1,2})\s*(?:min|mins|minutes?)?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    match = hMinRe.match(clean);
    if (match.hasMatch()) {
        const int h = match.captured(1).toInt();
        const QString mStr = match.captured(2);
        const int mins = mStr.toInt();
        if (mins >= 60) return std::nullopt;
        const int total = h * 60 + mins;
        const int rounded = static_cast<int>(std::round(static_cast<double>(total) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    // 3. Time separator: <sep> is one of : ; . ,
    // e.g. "7:30", "7.30", "7,30", "7;30", "7.05", "7.5", "7:5", "7,5", "0:50", "7:20"
    static const QRegularExpression sepRe(
        uR"(^(\d+)\s*([:;.,])\s*(\d+)\s*(?:h|hrs?|hours?)?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    match = sepRe.match(clean);
    if (match.hasMatch()) {
        const int h = match.captured(1).toInt();
        const QString minPart = match.captured(3);
        int mins = 0;
        if (minPart.length() == 1) {
            // One digit is tens of minutes (0 to 50 min). 6-9 is an error.
            const int tens = minPart.toInt();
            if (tens >= 6) return std::nullopt;
            mins = tens * 10;
        } else if (minPart.length() == 2) {
            // Two digits: 00-59. >= 60 is an error.
            mins = minPart.toInt();
            if (mins >= 60) return std::nullopt;
        } else {
            // 3 or more digits: error
            return std::nullopt;
        }
        const int total = h * 60 + mins;
        const int rounded = static_cast<int>(std::round(static_cast<double>(total) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    // 4. Bare integer hours or hours with unit: e.g. "8", "8 h", "8h", "45", "99 h"
    static const QRegularExpression hoursOnlyRe(
        uR"(^(\d+)\s*(?:h|hrs?|hours?)?$)"_s,
        QRegularExpression::CaseInsensitiveOption);
    match = hoursOnlyRe.match(clean);
    if (match.hasMatch()) {
        const int h = match.captured(1).toInt();
        const int total = h * 60;
        const int rounded = static_cast<int>(std::round(static_cast<double>(total) / 15.0)) * 15;
        return std::clamp(rounded, 60, 2400);
    }

    return std::nullopt;
}

}  // namespace

class GoalSpinBox : public QSpinBox {
public:
    explicit GoalSpinBox(QWidget* parent = nullptr) : QSpinBox(parent) {
        setRange(60, 2400);
        setSingleStep(15);
    }

    QString textFromValue(int val) const override {
        const int h = val / 60;
        const int m = val % 60;
        if (m == 0) {
            return tr("%1 h").arg(h);
        }
        return tr("%1 h %2 min").arg(h).arg(m);
    }

    int valueFromText(const QString& text) const override {
        const auto parsed = parseGoalMinutes(text);
        return parsed.value_or(value());
    }

    void fixup(QString& input) const override {
        input = textFromValue(value());
    }

    QValidator::State validate(QString& text, int& pos) const override {
        Q_UNUSED(pos);
        const QString clean = text.trimmed();
        if (clean.isEmpty()) return QValidator::Intermediate;
        const auto parsed = parseGoalMinutes(clean);
        if (parsed.has_value()) {
            return QValidator::Acceptable;
        }
        if (clean.endsWith(u':') || clean.endsWith(u';') ||
            clean.endsWith(u'.') || clean.endsWith(u',') ||
            clean.endsWith(u'h') || clean.endsWith(u'm') ||
            clean.endsWith(u"mi", Qt::CaseInsensitive)) {
            return QValidator::Intermediate;
        }
        return QValidator::Intermediate;
    }

protected:
    void focusOutEvent(QFocusEvent* event) override {
        interpretText();
        QSpinBox::focusOutEvent(event);
    }
};

QSpinBox* SettingsPage::studyGoalSpinBox() const {
    return m_studyGoalSpinBox;
}

SettingsPage::SettingsPage(Services::SettingsService& settings,
                           Theme::ThemeManager& themeMgr,
                           bool trayAvailable,
                           QWidget* parent,
                           DataToolsController* dataTools)
    : PageBase(tr("Settings"), parent),
      m_settings(settings),
      m_themeMgr(themeMgr),
      m_trayAvailable(trayAvailable),
      m_dataTools(dataTools) {
    setupUi();
    loadSettings();
}

void SettingsPage::setDataToolsController(DataToolsController* controller) {
    m_dataTools = controller;
}

void SettingsPage::setTrayAvailable(bool available) {
    if (m_trayAvailable == available) return;
    m_trayAvailable = available;
    updateCloseToTrayState();
}

void SettingsPage::updateCloseToTrayState() {
    if (!m_closeToTrayCheck) return;
    if (m_trayAvailable) {
        m_closeToTrayCheck->setEnabled(true);
        m_closeToTrayCheck->setToolTip(tr("Keep JobPrep running in the system tray when closing the window"));
    } else {
        m_closeToTrayCheck->setEnabled(false);
        m_closeToTrayCheck->setToolTip(tr("System tray is not available on this system"));
    }
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

    // --- Appearance Card ---
    auto* appearanceCard = new QFrame(container);
    appearanceCard->setObjectName(u"SettingsAppearanceCard"_s);
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
    m_themeCombo->setObjectName(u"SettingsThemeCombo"_s);
    m_themeCombo->setToolTip(tr("Select visual color theme"));
    m_themeCombo->addItem(tr("System Default"), u"system"_s);
    m_themeCombo->addItem(tr("Light"), u"light"_s);
    m_themeCombo->addItem(tr("Dark"), u"dark"_s);
    appForm->addRow(tr("Theme:"), m_themeCombo);

    m_accentCombo = new QComboBox(appearanceCard);
    m_accentCombo->setObjectName(u"SettingsAccentCombo"_s);
    m_accentCombo->setToolTip(tr("Select accent highlight color"));
    m_accentCombo->addItem(tr("Indigo"), u"indigo"_s);
    m_accentCombo->addItem(tr("Teal"), u"teal"_s);
    m_accentCombo->addItem(tr("Rose"), u"rose"_s);
    m_accentCombo->addItem(tr("Amber"), u"amber"_s);
    m_accentCombo->addItem(tr("Emerald"), u"emerald"_s);
    m_accentCombo->addItem(tr("Sky"), u"sky"_s);
    appForm->addRow(tr("Accent color:"), m_accentCombo);

    appearanceLayout->addLayout(appForm);
    containerLayout->addWidget(appearanceCard);

    // --- Preferences Card ---
    auto* prefsCard = new QFrame(container);
    prefsCard->setObjectName(u"SettingsPreferencesCard"_s);
    prefsCard->setProperty("card", true);
    auto* prefsLayout = new QVBoxLayout(prefsCard);
    prefsLayout->setContentsMargins(24, 24, 24, 24);
    prefsLayout->setSpacing(16);

    auto* prefsTitle = new QLabel(tr("Preferences"), prefsCard);
    prefsTitle->setStyleSheet(u"font-size: 12pt; font-weight: 600;"_s);
    prefsLayout->addWidget(prefsTitle);

    auto* prefsForm = new QFormLayout();
    prefsForm->setSpacing(12);

    // B1: Weekly study goal spinbox (60-2400 min, step 15)
    m_studyGoalSpinBox = new GoalSpinBox(prefsCard);
    m_studyGoalSpinBox->setObjectName(u"SettingsStudyGoalSpinBox"_s);
    m_studyGoalSpinBox->setToolTip(
        tr("Weekly study goal in hours and minutes (e.g. 8, 7.5, 7:30, 7;30, 450 min; bare numbers or decimals mean hours)"));
    prefsForm->addRow(tr("Weekly study goal:"), m_studyGoalSpinBox);

    // B4: First day of week combo (Monday = 1, Sunday = 7)
    m_weekStartCombo = new QComboBox(prefsCard);
    m_weekStartCombo->setObjectName(u"SettingsWeekStartCombo"_s);
    m_weekStartCombo->setToolTip(tr("First day of the week for calendar and study statistics"));
    m_weekStartCombo->addItem(tr("Monday"), 1);
    m_weekStartCombo->addItem(tr("Sunday"), 7);
    prefsForm->addRow(tr("First day of week:"), m_weekStartCombo);

    // B3: Close to tray checkbox
    m_closeToTrayCheck = new QCheckBox(tr("Minimize / Close to system tray"), prefsCard);
    m_closeToTrayCheck->setObjectName(u"SettingsCloseToTrayCheck"_s);
    prefsForm->addRow(QString(), m_closeToTrayCheck);

    prefsLayout->addLayout(prefsForm);

    // B2: Reminder Lead Times Editor
    auto* leadSectionTitle = new QLabel(tr("Interview Reminder Lead Times"), prefsCard);
    leadSectionTitle->setStyleSheet(u"font-size: 11pt; font-weight: 600;"_s);
    prefsLayout->addWidget(leadSectionTitle);

    m_leadList = new QListWidget(prefsCard);
    m_leadList->setObjectName(u"SettingsLeadList"_s);
    m_leadList->setToolTip(tr("Active interview reminder notifications before start time"));
    prefsLayout->addWidget(m_leadList);

    auto* leadControlRow = new QHBoxLayout();
    leadControlRow->setSpacing(8);

    m_leadPresetCombo = new QComboBox(prefsCard);
    m_leadPresetCombo->setObjectName(u"SettingsLeadPresetCombo"_s);
    m_leadPresetCombo->setToolTip(tr("Select a preset lead time or choose Custom"));
    m_leadPresetCombo->addItem(tr("15 minutes"), 15);
    m_leadPresetCombo->addItem(tr("30 minutes"), 30);
    m_leadPresetCombo->addItem(tr("1 hour"), 60);
    m_leadPresetCombo->addItem(tr("2 hours"), 120);
    m_leadPresetCombo->addItem(tr("1 day"), 1440);
    m_leadPresetCombo->addItem(tr("2 days"), 2880);
    m_leadPresetCombo->addItem(tr("Custom..."), -1);
    leadControlRow->addWidget(m_leadPresetCombo);

    m_customLeadSpinBox = new QSpinBox(prefsCard);
    m_customLeadSpinBox->setObjectName(u"SettingsCustomLeadSpinBox"_s);
    m_customLeadSpinBox->setRange(1, 10080);
    m_customLeadSpinBox->setValue(90);
    m_customLeadSpinBox->setSuffix(tr(" min"));
    m_customLeadSpinBox->setToolTip(tr("Custom reminder lead time in minutes (1 - 10080)"));
    m_customLeadSpinBox->setEnabled(false);
    leadControlRow->addWidget(m_customLeadSpinBox);

    m_addLeadBtn = new QPushButton(tr("Add lead"), prefsCard);
    m_addLeadBtn->setObjectName(u"SettingsAddLeadBtn"_s);
    m_addLeadBtn->setToolTip(tr("Add reminder lead time to the list"));
    m_addLeadBtn->setAutoDefault(true);
    m_addLeadBtn->installEventFilter(this);
    leadControlRow->addWidget(m_addLeadBtn);

    m_removeLeadBtn = new QPushButton(tr("Remove selected"), prefsCard);
    m_removeLeadBtn->setObjectName(u"SettingsRemoveLeadBtn"_s);
    m_removeLeadBtn->setToolTip(tr("Remove the selected reminder lead time"));
    m_removeLeadBtn->setAutoDefault(true);
    leadControlRow->addWidget(m_removeLeadBtn);

    m_resetLeadBtn = new QPushButton(tr("Reset to default"), prefsCard);
    m_resetLeadBtn->setObjectName(u"SettingsResetLeadBtn"_s);
    m_resetLeadBtn->setToolTip(tr("Reset reminder lead times to defaults (1 day and 1 hour)"));
    m_resetLeadBtn->setAutoDefault(true);
    leadControlRow->addWidget(m_resetLeadBtn);

    prefsLayout->addLayout(leadControlRow);
    containerLayout->addWidget(prefsCard);

    // --- Data & Backups Card (C8) ---
    m_dataCard = new QFrame(container);
    m_dataCard->setObjectName(u"SettingsDataCard"_s);
    m_dataCard->setProperty("card", true);
    auto* dataLayout = new QVBoxLayout(m_dataCard);
    dataLayout->setContentsMargins(24, 24, 24, 24);
    dataLayout->setSpacing(16);

    auto* dataTitle = new QLabel(tr("Data & Backups"), m_dataCard);
    dataTitle->setStyleSheet(u"font-size: 12pt; font-weight: 600;"_s);
    dataLayout->addWidget(dataTitle);

    auto* dataDesc = new QLabel(
        tr("Export applications and interviews, create full JSON backups, or load sample preparation tracks."),
        m_dataCard);
    dataDesc->setWordWrap(true);
    dataLayout->addWidget(dataDesc);

    auto* buttonsGrid = new QGridLayout();
    buttonsGrid->setSpacing(12);

    m_exportCsvBtn = new QPushButton(tr("Export Applications (CSV)"), m_dataCard);
    m_exportCsvBtn->setObjectName(u"SettingsExportCsvBtn"_s);
    m_exportCsvBtn->setToolTip(tr("Export job applications as a CSV spreadsheet"));
    m_exportCsvBtn->setAutoDefault(true);
    m_exportCsvBtn->installEventFilter(this);
    buttonsGrid->addWidget(m_exportCsvBtn, 0, 0);

    m_exportIcsBtn = new QPushButton(tr("Export Interviews (ICS)"), m_dataCard);
    m_exportIcsBtn->setObjectName(u"SettingsExportIcsBtn"_s);
    m_exportIcsBtn->setToolTip(tr("Export upcoming and past interviews as an iCalendar file"));
    m_exportIcsBtn->setAutoDefault(true);
    m_exportIcsBtn->installEventFilter(this);
    buttonsGrid->addWidget(m_exportIcsBtn, 0, 1);

    m_backupJsonBtn = new QPushButton(tr("Backup Database (JSON)"), m_dataCard);
    m_backupJsonBtn->setObjectName(u"SettingsBackupJsonBtn"_s);
    m_backupJsonBtn->setToolTip(
        tr("Create a complete JSON backup of all tracks, topics, applications, and interviews"));
    m_backupJsonBtn->setAutoDefault(true);
    m_backupJsonBtn->installEventFilter(this);
    buttonsGrid->addWidget(m_backupJsonBtn, 1, 0);

    m_restoreJsonBtn = new QPushButton(tr("Restore Database..."), m_dataCard);
    m_restoreJsonBtn->setObjectName(u"SettingsRestoreJsonBtn"_s);
    m_restoreJsonBtn->setToolTip(
        tr("Restore database from a JSON backup file (replaces current data)"));
    m_restoreJsonBtn->setAutoDefault(true);
    m_restoreJsonBtn->installEventFilter(this);
    buttonsGrid->addWidget(m_restoreJsonBtn, 1, 1);

    m_loadSampleBtn = new QPushButton(tr("Load Sample Data"), m_dataCard);
    m_loadSampleBtn->setObjectName(u"SettingsLoadSampleBtn"_s);
    m_loadSampleBtn->setToolTip(tr("Load standard interview preparation tracks and topics"));
    m_loadSampleBtn->setAutoDefault(true);
    m_loadSampleBtn->installEventFilter(this);
    buttonsGrid->addWidget(m_loadSampleBtn, 2, 0);

    m_openFolderBtn = new QPushButton(tr("Open Data Folder"), m_dataCard);
    m_openFolderBtn->setObjectName(u"SettingsOpenFolderBtn"_s);
    m_openFolderBtn->setToolTip(
        tr("Open the local folder containing application data and SQLite database"));
    m_openFolderBtn->setAutoDefault(true);
    m_openFolderBtn->installEventFilter(this);
    buttonsGrid->addWidget(m_openFolderBtn, 2, 1);

    dataLayout->addLayout(buttonsGrid);
    containerLayout->addWidget(m_dataCard);

    containerLayout->addStretch(1);
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);

    // --- Tab Order Setup ---
    QWidget::setTabOrder(m_themeCombo, m_accentCombo);
    QWidget::setTabOrder(m_accentCombo, m_studyGoalSpinBox);
    QWidget::setTabOrder(m_studyGoalSpinBox, m_weekStartCombo);
    QWidget::setTabOrder(m_weekStartCombo, m_closeToTrayCheck);
    QWidget::setTabOrder(m_closeToTrayCheck, m_leadList);
    QWidget::setTabOrder(m_leadList, m_leadPresetCombo);
    QWidget::setTabOrder(m_leadPresetCombo, m_customLeadSpinBox);
    QWidget::setTabOrder(m_customLeadSpinBox, m_addLeadBtn);
    QWidget::setTabOrder(m_addLeadBtn, m_removeLeadBtn);
    QWidget::setTabOrder(m_removeLeadBtn, m_resetLeadBtn);
    QWidget::setTabOrder(m_resetLeadBtn, m_exportCsvBtn);
    QWidget::setTabOrder(m_exportCsvBtn, m_exportIcsBtn);
    QWidget::setTabOrder(m_exportIcsBtn, m_backupJsonBtn);
    QWidget::setTabOrder(m_backupJsonBtn, m_restoreJsonBtn);
    QWidget::setTabOrder(m_restoreJsonBtn, m_loadSampleBtn);
    QWidget::setTabOrder(m_loadSampleBtn, m_openFolderBtn);

    // Wire Data Section Actions
    connect(m_exportCsvBtn, &QPushButton::clicked, this, [this] {
        if (m_dataTools) m_dataTools->exportCsv(this);
    });
    connect(m_exportIcsBtn, &QPushButton::clicked, this, [this] {
        if (m_dataTools) m_dataTools->exportIcs(this);
    });
    connect(m_backupJsonBtn, &QPushButton::clicked, this, [this] {
        if (m_dataTools) m_dataTools->backupJson(this);
    });
    connect(m_restoreJsonBtn, &QPushButton::clicked, this, [this] {
        if (m_dataTools) m_dataTools->restoreJson(this);
    });
    connect(m_loadSampleBtn, &QPushButton::clicked, this, [this] {
        if (m_dataTools) m_dataTools->loadSampleData(this);
    });
    connect(m_openFolderBtn, &QPushButton::clicked, this, [this] {
        if (m_dataTools) m_dataTools->openDataFolder(this);
    });

    // --- Signal Wiring ---
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_syncingTheme) return;
        const QString modeStr = m_themeCombo->itemData(index).toString();
        m_themeMgr.setThemeMode(Theme::Tokens::themeModeFromString(modeStr));
    });

    connect(m_accentCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_syncingTheme) return;
        const QString accentStr = m_accentCombo->itemData(index).toString();
        m_themeMgr.setAccentPreset(Theme::Tokens::accentFromString(accentStr));
    });

    connect(&m_themeMgr, &Theme::ThemeManager::themeChanged, this, [this]() {
        const QString modeStr = Theme::Tokens::themeModeToString(m_themeMgr.currentMode());
        const int themeIdx = m_themeCombo->findData(modeStr);
        if (themeIdx >= 0 && m_themeCombo->currentIndex() != themeIdx) {
            m_syncingTheme = true;
            m_themeCombo->setCurrentIndex(themeIdx);
            m_syncingTheme = false;
        }

        const QString accentStr = Theme::Tokens::accentToString(m_themeMgr.currentAccent());
        const int accentIdx = m_accentCombo->findData(accentStr);
        if (accentIdx >= 0 && m_accentCombo->currentIndex() != accentIdx) {
            m_syncingTheme = true;
            m_accentCombo->setCurrentIndex(accentIdx);
            m_syncingTheme = false;
        }
    });

    connect(m_studyGoalSpinBox, &QSpinBox::valueChanged, this, [this](int minutes) {
        m_settings.setWeeklyGoalMinutes(minutes);
    });

    connect(m_weekStartCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const int day = m_weekStartCombo->itemData(index).toInt();
        m_settings.setFirstDayOfWeek(day);
    });

    connect(m_closeToTrayCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setCloseToTray(checked);
    });

    connect(m_leadPresetCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const bool isCustom = m_leadPresetCombo->itemData(index).toInt() == -1;
        m_customLeadSpinBox->setEnabled(isCustom);
    });

    connect(m_addLeadBtn, &QPushButton::clicked, this, [this]() {
        int minutes = m_leadPresetCombo->currentData().toInt();
        if (minutes == -1) {
            minutes = m_customLeadSpinBox->value();
        }
        auto current = m_settings.interviewLeadMinutes();
        if (!current.contains(minutes)) {
            current.append(minutes);
            std::sort(current.begin(), current.end(), std::greater<int>());
            m_settings.setInterviewLeadMinutes(current);
            refreshLeadList();
        }
    });

    connect(m_removeLeadBtn, &QPushButton::clicked, this, [this]() {
        auto current = m_settings.interviewLeadMinutes();
        if (current.size() <= 1) return;
        const int row = m_leadList->currentRow();
        if (row >= 0 && row < current.size()) {
            current.removeAt(row);
            m_settings.setInterviewLeadMinutes(current);
            refreshLeadList();
        }
    });

    connect(m_resetLeadBtn, &QPushButton::clicked, this, [this]() {
        m_settings.setInterviewLeadMinutes({1440, 60});
        refreshLeadList();
    });

    connect(m_leadList, &QListWidget::currentRowChanged, this, [this](int row) {
        m_removeLeadBtn->setEnabled(row >= 0 && m_leadList->count() > 1);
    });

    connect(&m_settings, &Services::SettingsService::interviewLeadMinutesChanged,
            this, &SettingsPage::refreshLeadList);
}

void SettingsPage::refreshLeadList() {
    m_leadList->clear();
    const auto leads = m_settings.interviewLeadMinutes();
    for (int m : leads) {
        auto* item = new QListWidgetItem(FormatHelper::formatLeadTime(m), m_leadList);
        item->setData(Qt::UserRole, m);
    }
    if (!leads.isEmpty()) {
        m_leadList->setCurrentRow(0);
    }
    m_removeLeadBtn->setEnabled(leads.size() > 1 && m_leadList->currentRow() >= 0);
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

    // B1: Weekly study goal (minutes: 60-2400)
    m_studyGoalSpinBox->setValue(m_settings.weeklyGoalMinutes());

    // B4: First day of week (1=Monday, 7=Sunday; invalid/legacy treated as Monday)
    const int day = m_settings.firstDayOfWeek();
    const int dayIdx = m_weekStartCombo->findData(day);
    if (dayIdx >= 0) {
        m_weekStartCombo->setCurrentIndex(dayIdx);
    } else {
        m_weekStartCombo->setCurrentIndex(0);
    }

    // B3: Close to tray
    m_closeToTrayCheck->setChecked(m_settings.closeToTray());
    updateCloseToTrayState();

    // B2: Lead times list
    refreshLeadList();
}

bool SettingsPage::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_addLeadBtn && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            m_addLeadBtn->click();
            return true;
        }
    }
    return PageBase::eventFilter(watched, event);
}

}  // namespace JobPrep::Ui::Pages
