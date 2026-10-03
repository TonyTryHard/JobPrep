#pragma once

#include <QObject>
#include "ui/theme/Tokens.h"

namespace JobPrep::Services {
class SettingsService;
}

namespace JobPrep::Ui::Theme {

/// Manages application theming, stylesheet preprocessing, and runtime switching.
class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(Services::SettingsService& settings, QObject* parent = nullptr);
    ~ThemeManager() override = default;

    ThemeMode currentMode() const;
    AccentPreset currentAccent() const;
    bool isDark() const;

    void setThemeMode(ThemeMode mode);
    void setAccentPreset(AccentPreset preset);

    ColorTokens currentTokens() const;

signals:
    void themeChanged();

private:
    void applyTheme();
    QString processStylesheet(const QString& rawQss, const ColorTokens& tokens);
    bool detectSystemDark() const;

    Services::SettingsService& m_settings;
    ThemeMode m_mode{ThemeMode::System};
    AccentPreset m_accent{AccentPreset::Indigo};
};

}  // namespace JobPrep::Ui::Theme
