#pragma once

#include <memory>
#include <QByteArray>
#include <QObject>
#include <QString>

class QSettings;

namespace JobPrep::Services {

/// Typed wrapper around QSettings for application configuration.
class SettingsService : public QObject {
    Q_OBJECT

public:
    explicit SettingsService(QObject* parent = nullptr);
    explicit SettingsService(std::unique_ptr<QSettings> settings, QObject* parent = nullptr);
    ~SettingsService() override;

    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

    QByteArray windowState() const;
    void setWindowState(const QByteArray& state);

    bool isSidebarCollapsed() const;
    void setSidebarCollapsed(bool collapsed);

    QString themeMode() const;
    void setThemeMode(const QString& mode);

    QString accentPreset() const;
    void setAccentPreset(const QString& preset);

    int weeklyGoalMinutes() const;
    void setWeeklyGoalMinutes(int minutes);

    int firstDayOfWeek() const;
    void setFirstDayOfWeek(int day);
    Qt::DayOfWeek firstDayOfWeekEnum() const;
    static Qt::DayOfWeek toDayOfWeek(int day);

    QList<int> interviewLeadMinutes() const;
    void setInterviewLeadMinutes(const QList<int>& minutes);

    bool closeToTray() const;
    void setCloseToTray(bool enabled);

    bool trayHintShown() const;
    void setTrayHintShown(bool shown);

    bool sampleDataPrompted() const;
    void setSampleDataPrompted(bool prompted);

    bool calendarLayerInterviews() const;
    void setCalendarLayerInterviews(bool enabled);

    bool calendarLayerFollowUps() const;
    void setCalendarLayerFollowUps(bool enabled);

    bool calendarLayerTargets() const;
    void setCalendarLayerTargets(bool enabled);

    QByteArray calendarSplitterState() const;
    void setCalendarSplitterState(const QByteArray& state);

    QByteArray studySplitterState() const;
    void setStudySplitterState(const QByteArray& state);

    QString studyViewMode() const;
    void setStudyViewMode(const QString& mode);

    int studyTrackFilter() const;
    void setStudyTrackFilter(int trackId);

signals:
    void themeModeChanged(const QString& mode);
    void accentPresetChanged(const QString& preset);
    void sidebarCollapsedChanged(bool collapsed);
    void weeklyGoalMinutesChanged(int minutes);
    void firstDayOfWeekChanged(int day);
    void interviewLeadMinutesChanged(const QList<int>& minutes);
    void closeToTrayChanged(bool enabled);
    void trayHintShownChanged(bool shown);
    void calendarLayerInterviewsChanged(bool enabled);
    void calendarLayerFollowUpsChanged(bool enabled);
    void calendarLayerTargetsChanged(bool enabled);
    void studySplitterStateChanged(const QByteArray& state);
    void studyViewModeChanged(const QString& mode);
    void studyTrackFilterChanged(int trackId);

private:
    std::unique_ptr<QSettings> m_settings;
};

}  // namespace JobPrep::Services
