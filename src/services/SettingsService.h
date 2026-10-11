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

    bool closeToTray() const;
    void setCloseToTray(bool enabled);

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

signals:
    void themeModeChanged(const QString& mode);
    void accentPresetChanged(const QString& preset);
    void sidebarCollapsedChanged(bool collapsed);
    void weeklyGoalMinutesChanged(int minutes);
    void firstDayOfWeekChanged(int day);
    void closeToTrayChanged(bool enabled);
    void calendarLayerInterviewsChanged(bool enabled);
    void calendarLayerFollowUpsChanged(bool enabled);
    void calendarLayerTargetsChanged(bool enabled);

private:
    std::unique_ptr<QSettings> m_settings;
};

}  // namespace JobPrep::Services
