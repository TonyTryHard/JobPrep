#pragma once

#include <QCoreApplication>
#include <QSettings>
#include <QString>
#include <QTemporaryDir>

using namespace Qt::StringLiterals;

namespace JobPrep::Tests {

/// Configures QSettings to write into an isolated temporary directory in IniFormat
/// and ensures organizationName and applicationName are set, preventing test collisions
/// and Windows registry/AccessError issues.
class TestSettingsScope {
public:
    explicit TestSettingsScope(const QString& appPrefix = QString()) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tempDir.path());
        const QString appName = appPrefix.isEmpty()
            ? u"JobPrepTest_%1"_s.arg(QCoreApplication::applicationPid())
            : u"%1_%2"_s.arg(appPrefix).arg(QCoreApplication::applicationPid());
        QCoreApplication::setOrganizationName(u"JobPrepTest"_s);
        QCoreApplication::setApplicationName(appName);
        m_origXdgData = qgetenv("XDG_DATA_HOME");
        m_origXdgConfig = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_DATA_HOME", m_tempDir.filePath(u"data"_s).toLocal8Bit());
        qputenv("XDG_CONFIG_HOME", m_tempDir.filePath(u"config"_s).toLocal8Bit());
    }

    ~TestSettingsScope() {
        if (m_origXdgData.isEmpty()) {
            qunsetenv("XDG_DATA_HOME");
        } else {
            qputenv("XDG_DATA_HOME", m_origXdgData);
        }
        if (m_origXdgConfig.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", m_origXdgConfig);
        }
    }

    QString path() const { return m_tempDir.path(); }
    bool isValid() const { return m_tempDir.isValid(); }

private:
    QTemporaryDir m_tempDir;
    QByteArray m_origXdgData;
    QByteArray m_origXdgConfig;
};

}  // namespace JobPrep::Tests
