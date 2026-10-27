#include "services/SettingsService.h"
#include <QSettings>

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

QList<int> sanitizeLeadMinutes(const QList<int>& raw) {
    QList<int> valid;
    for (int v : raw) {
        if (v > 0 && !valid.contains(v)) {
            valid.append(v);
        }
    }
    std::sort(valid.begin(), valid.end(), std::greater<int>());
    if (valid.isEmpty()) {
        return {1440, 60};
    }
    return valid;
}

}  // namespace

SettingsService::SettingsService(QObject* parent)
    : SettingsService(std::make_unique<QSettings>(), parent) {}

SettingsService::SettingsService(std::unique_ptr<QSettings> settings, QObject* parent)
    : QObject(parent), m_settings(std::move(settings)) {}

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

Qt::DayOfWeek SettingsService::toDayOfWeek(int day) {
    if (day == 7) {
        return Qt::Sunday;
    }
    return Qt::Monday;
}

Qt::DayOfWeek SettingsService::firstDayOfWeekEnum() const {
    return toDayOfWeek(firstDayOfWeek());
}

QList<int> SettingsService::interviewLeadMinutes() const {
    const auto val = m_settings->value(u"reminders/interviewLeadMinutes"_s);
    if (!val.isValid()) {
        return {1440, 60};
    }
    QList<int> list;
    if (val.canConvert<QVariantList>()) {
        const auto varList = val.toList();
        for (const auto& item : varList) {
            bool ok = false;
            int v = item.toInt(&ok);
            if (ok) list.append(v);
        }
    }
    return sanitizeLeadMinutes(list);
}

void SettingsService::setInterviewLeadMinutes(const QList<int>& minutes) {
    const auto sanitized = sanitizeLeadMinutes(minutes);
    if (interviewLeadMinutes() == sanitized) return;
    QVariantList varList;
    for (int m : sanitized) {
        varList.append(m);
    }
    m_settings->setValue(u"reminders/interviewLeadMinutes"_s, varList);
    emit interviewLeadMinutesChanged(sanitized);
}

bool SettingsService::closeToTray() const {
    return m_settings->value(u"app/closeToTray"_s, false).toBool();
}

void SettingsService::setCloseToTray(bool enabled) {
    if (closeToTray() == enabled) return;
    m_settings->setValue(u"app/closeToTray"_s, enabled);
    emit closeToTrayChanged(enabled);
}

bool SettingsService::trayHintShown() const {
    return m_settings->value(u"app/trayHintShown"_s, false).toBool();
}

void SettingsService::setTrayHintShown(bool shown) {
    if (trayHintShown() == shown) return;
    m_settings->setValue(u"app/trayHintShown"_s, shown);
    emit trayHintShownChanged(shown);
}

bool SettingsService::sampleDataPrompted() const {
    // The first-launch "Load sample data?" question is asked only once (SPEC §3.7).
    return m_settings->value(u"app/sampleDataPrompted"_s, false).toBool();
}

void SettingsService::setSampleDataPrompted(bool prompted) {
    if (sampleDataPrompted() == prompted) return;
    m_settings->setValue(u"app/sampleDataPrompted"_s, prompted);
}

bool SettingsService::calendarLayerInterviews() const {
    return m_settings->value(u"calendar/layerInterviews"_s, true).toBool();
}

void SettingsService::setCalendarLayerInterviews(bool enabled) {
    if (calendarLayerInterviews() == enabled) return;
    m_settings->setValue(u"calendar/layerInterviews"_s, enabled);
    emit calendarLayerInterviewsChanged(enabled);
}

bool SettingsService::calendarLayerFollowUps() const {
    return m_settings->value(u"calendar/layerFollowUps"_s, true).toBool();
}

void SettingsService::setCalendarLayerFollowUps(bool enabled) {
    if (calendarLayerFollowUps() == enabled) return;
    m_settings->setValue(u"calendar/layerFollowUps"_s, enabled);
    emit calendarLayerFollowUpsChanged(enabled);
}

bool SettingsService::calendarLayerTargets() const {
    return m_settings->value(u"calendar/layerTargets"_s, true).toBool();
}

void SettingsService::setCalendarLayerTargets(bool enabled) {
    if (calendarLayerTargets() == enabled) return;
    m_settings->setValue(u"calendar/layerTargets"_s, enabled);
    emit calendarLayerTargetsChanged(enabled);
}

QByteArray SettingsService::calendarSplitterState() const {
    return m_settings->value(u"calendar/splitterState"_s).toByteArray();
}

void SettingsService::setCalendarSplitterState(const QByteArray& state) {
    m_settings->setValue(u"calendar/splitterState"_s, state);
}

QByteArray SettingsService::studySplitterState() const {
    return m_settings->value(u"study/splitterState"_s).toByteArray();
}

void SettingsService::setStudySplitterState(const QByteArray& state) {
    if (studySplitterState() == state) return;
    m_settings->setValue(u"study/splitterState"_s, state);
    emit studySplitterStateChanged(state);
}

QString SettingsService::studyViewMode() const {
    const QString mode = m_settings->value(u"study/viewMode"_s, u"board"_s).toString();
    if (mode == u"board"_s || mode == u"list"_s) {
        return mode;
    }
    return u"board"_s;
}

void SettingsService::setStudyViewMode(const QString& mode) {
    if (studyViewMode() == mode) return;
    m_settings->setValue(u"study/viewMode"_s, mode);
    emit studyViewModeChanged(mode);
}

int SettingsService::studyTrackFilter() const {
    return m_settings->value(u"study/trackFilter"_s, -1).toInt();
}

void SettingsService::setStudyTrackFilter(int trackId) {
    if (studyTrackFilter() == trackId) return;
    m_settings->setValue(u"study/trackFilter"_s, trackId);
    emit studyTrackFilterChanged(trackId);
}

}  // namespace JobPrep::Services
