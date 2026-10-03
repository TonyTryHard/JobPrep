#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

namespace JobPrep::Ui::Theme {

/// Theme mode setting.
enum class ThemeMode {
    System,
    Light,
    Dark,
};

/// Predefined accent colors per SPEC §7.6.
enum class AccentPreset {
    Indigo,
    Teal,
    Rose,
    Amber,
    Emerald,
    Sky,
};

/// Concrete color tokens resolved for a theme mode and accent color.
struct ColorTokens {
    QColor bg;
    QColor surface;
    QColor surfaceAlt;
    QColor border;
    QColor text;
    QColor textMuted;
    QColor accent;
    QColor accentHover;
    QColor accentPressed;
    QColor accentText;
    QColor danger;
    QColor warning;
    QColor success;

    /// Build a consistent QPalette for standard Qt widgets.
    QPalette toPalette() const;
};

/// Resolves color tokens based on theme mode and accent preset.
class Tokens {
public:
    static ColorTokens get(bool isDark, AccentPreset accent);
    static AccentPreset accentFromString(const QString& name);
    static QString accentToString(AccentPreset accent);
    static ThemeMode themeModeFromString(const QString& name);
    static QString themeModeToString(ThemeMode mode);
};

}  // namespace JobPrep::Ui::Theme
