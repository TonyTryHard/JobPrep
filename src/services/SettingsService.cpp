#include "services/SettingsService.h"
#include <QSettings>

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

SettingsService::SettingsService(QObject* parent)
    : QObject(parent), m_settings(std::make_unique<QSettings>()) {}

SettingsService::~SettingsService() = default;

QByteArray SettingsService::windowGeometry() const {
    return m_settings->value(u"window/geometry"_s).toByteArray();
}

void SettingsService::setWindowGeometry(const QByteArray& geometry) {
    m_settings->setValue(u"window/geometry"_s, geometry);
}

QByteArray SettingsService::windowState() const {
    return m_settings->value(u"window/state"_s).toByteArray();
}

void SettingsService::setWindowState(const QByteArray& state) {
    m_settings->setValue(u"window/state"_s, state);
}

bool SettingsService::isSidebarCollapsed() const {
    return m_settings->value(u"ui/sidebarCollapsed"_s, false).toBool();
}

void SettingsService::setSidebarCollapsed(bool collapsed) {
    if (isSidebarCollapsed() == collapsed) return;
    m_settings->setValue(u"ui/sidebarCollapsed"_s, collapsed);
    emit sidebarCollapsedChanged(collapsed);
}

QString SettingsService::themeMode() const {
    return m_settings->value(u"ui/themeMode"_s, u"system"_s).toString();
}

void SettingsService::setThemeMode(const QString& mode) {
    if (themeMode() == mode) return;
    m_settings->setValue(u"ui/themeMode"_s, mode);
    emit themeModeChanged(mode);
}

QString SettingsService::accentPreset() const {
    return m_settings->value(u"ui/accentPreset"_s, u"indigo"_s).toString();
}

void SettingsService::setAccentPreset(const QString& preset) {
    if (accentPreset() == preset) return;
    m_settings->setValue(u"ui/accentPreset"_s, preset);
    emit accentPresetChanged(preset);
}

int SettingsService::weeklyGoalMinutes() const {
    // Default 6 h = 360 min per SPEC §3.6
    return m_settings->value(u"study/weeklyGoalMinutes"_s, 360).toInt();
}

void SettingsService::setWeeklyGoalMinutes(int minutes) {
    if (weeklyGoalMinutes() == minutes) return;
    m_settings->setValue(u"study/weeklyGoalMinutes"_s, minutes);
    emit weeklyGoalMinutesChanged(minutes);
}

int SettingsService::firstDayOfWeek() const {
    // 1 = Monday per SPEC §3.4 C1
    return m_settings->value(u"calendar/firstDayOfWeek"_s, 1).toInt();
}

void SettingsService::setFirstDayOfWeek(int day) {
    if (firstDayOfWeek() == day) return;
    m_settings->setValue(u"calendar/firstDayOfWeek"_s, day);
    emit firstDayOfWeekChanged(day);
}

bool SettingsService::closeToTray() const {
    return m_settings->value(u"app/closeToTray"_s, false).toBool();
}

void SettingsService::setCloseToTray(bool enabled) {
    if (closeToTray() == enabled) return;
    m_settings->setValue(u"app/closeToTray"_s, enabled);
    emit closeToTrayChanged(enabled);
}

bool SettingsService::sampleDataPrompted() const {
    // The first-launch "Load sample data?" question is asked only once (SPEC §3.7).
    return m_settings->value(u"app/sampleDataPrompted"_s, false).toBool();
}

void SettingsService::setSampleDataPrompted(bool prompted) {
    if (sampleDataPrompted() == prompted) return;
    m_settings->setValue(u"app/sampleDataPrompted"_s, prompted);
}

}  // namespace JobPrep::Services
