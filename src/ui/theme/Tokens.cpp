#include "ui/theme/Tokens.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Theme {

QPalette ColorTokens::toPalette() const {
    QPalette pal;
    pal.setColor(QPalette::Window, bg);
    pal.setColor(QPalette::WindowText, text);
    pal.setColor(QPalette::Base, surface);
    pal.setColor(QPalette::AlternateBase, surfaceAlt);
    pal.setColor(QPalette::ToolTipBase, surface);
    pal.setColor(QPalette::ToolTipText, text);
    pal.setColor(QPalette::Text, text);
    pal.setColor(QPalette::Button, surface);
    pal.setColor(QPalette::ButtonText, text);
    pal.setColor(QPalette::BrightText, danger);
    pal.setColor(QPalette::Highlight, accent);
    pal.setColor(QPalette::HighlightedText, accentText);
    pal.setColor(QPalette::PlaceholderText, textMuted);
    pal.setColor(QPalette::Midlight, surfaceAlt);
    pal.setColor(QPalette::Dark, border);
    pal.setColor(QPalette::Mid, border);
    pal.setColor(QPalette::Shadow, border.darker(110));

    // Disabled state
    pal.setColor(QPalette::Disabled, QPalette::WindowText, textMuted);
    pal.setColor(QPalette::Disabled, QPalette::Text, textMuted);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, textMuted);
    pal.setColor(QPalette::Disabled, QPalette::Highlight, surfaceAlt);
    pal.setColor(QPalette::Disabled, QPalette::HighlightedText, textMuted);

    return pal;
}

ColorTokens Tokens::get(bool isDark, AccentPreset accent) {
    ColorTokens t;
    if (isDark) {
        t.bg = QColor(u"#0F1117"_s);
        t.surface = QColor(u"#171A23"_s);
        t.surfaceAlt = QColor(u"#1E2230"_s);
        t.border = QColor(u"#2A2F3F"_s);
        t.text = QColor(u"#E8EAF2"_s);
        t.textMuted = QColor(u"#9AA3B8"_s);
        t.danger = QColor(u"#F87171"_s);
        t.warning = QColor(u"#FBBF24"_s);
        t.success = QColor(u"#34D399"_s);
        t.accentText = QColor(u"#FFFFFF"_s);

        switch (accent) {
            case AccentPreset::Indigo:
                t.accent = QColor(u"#7C8AFF"_s);
                t.accentHover = QColor(u"#939EFF"_s);
                t.accentPressed = QColor(u"#6774FF"_s);
                break;
            case AccentPreset::Teal:
                t.accent = QColor(u"#2DD4BF"_s);
                t.accentHover = QColor(u"#5EEAD4"_s);
                t.accentPressed = QColor(u"#14B8A6"_s);
                break;
            case AccentPreset::Rose:
                t.accent = QColor(u"#FB7185"_s);
                t.accentHover = QColor(u"#FDA4AF"_s);
                t.accentPressed = QColor(u"#F43F5E"_s);
                break;
            case AccentPreset::Amber:
                t.accent = QColor(u"#FBBF24"_s);
                t.accentHover = QColor(u"#FCD34D"_s);
                t.accentPressed = QColor(u"#F59E0B"_s);
                break;
            case AccentPreset::Emerald:
                t.accent = QColor(u"#34D399"_s);
                t.accentHover = QColor(u"#6EE7B7"_s);
                t.accentPressed = QColor(u"#10B981"_s);
                break;
            case AccentPreset::Sky:
                t.accent = QColor(u"#38BDF8"_s);
                t.accentHover = QColor(u"#7DD3FC"_s);
                t.accentPressed = QColor(u"#0EA5E9"_s);
                break;
        }
    } else {
        t.bg = QColor(u"#F6F7FB"_s);
        t.surface = QColor(u"#FFFFFF"_s);
        t.surfaceAlt = QColor(u"#EEF0F6"_s);
        t.border = QColor(u"#E3E6EF"_s);
        t.text = QColor(u"#1F2430"_s);
        t.textMuted = QColor(u"#6B7385"_s);
        t.danger = QColor(u"#EF4444"_s);
        t.warning = QColor(u"#F59E0B"_s);
        t.success = QColor(u"#10B981"_s);
        t.accentText = QColor(u"#FFFFFF"_s);

        switch (accent) {
            case AccentPreset::Indigo:
                t.accent = QColor(u"#5B6CFF"_s);
                t.accentHover = QColor(u"#4859EC"_s);
                t.accentPressed = QColor(u"#3A4AD6"_s);
                break;
            case AccentPreset::Teal:
                t.accent = QColor(u"#14B8A6"_s);
                t.accentHover = QColor(u"#0D9488"_s);
                t.accentPressed = QColor(u"#0F766E"_s);
                break;
            case AccentPreset::Rose:
                t.accent = QColor(u"#F43F5E"_s);
                t.accentHover = QColor(u"#E11D48"_s);
                t.accentPressed = QColor(u"#BE123C"_s);
                break;
            case AccentPreset::Amber:
                t.accent = QColor(u"#F59E0B"_s);
                t.accentHover = QColor(u"#D97706"_s);
                t.accentPressed = QColor(u"#B45309"_s);
                break;
            case AccentPreset::Emerald:
                t.accent = QColor(u"#10B981"_s);
                t.accentHover = QColor(u"#059669"_s);
                t.accentPressed = QColor(u"#047857"_s);
                break;
            case AccentPreset::Sky:
                t.accent = QColor(u"#0EA5E9"_s);
                t.accentHover = QColor(u"#0284C7"_s);
                t.accentPressed = QColor(u"#0369A1"_s);
                break;
        }
    }
    return t;
}

AccentPreset Tokens::accentFromString(const QString& name) {
    if (name == u"teal") return AccentPreset::Teal;
    if (name == u"rose") return AccentPreset::Rose;
    if (name == u"amber") return AccentPreset::Amber;
    if (name == u"emerald") return AccentPreset::Emerald;
    if (name == u"sky") return AccentPreset::Sky;
    return AccentPreset::Indigo;
}

QString Tokens::accentToString(AccentPreset accent) {
    switch (accent) {
        case AccentPreset::Indigo:
            return u"indigo"_s;
        case AccentPreset::Teal:
            return u"teal"_s;
        case AccentPreset::Rose:
            return u"rose"_s;
        case AccentPreset::Amber:
            return u"amber"_s;
        case AccentPreset::Emerald:
            return u"emerald"_s;
        case AccentPreset::Sky:
            return u"sky"_s;
    }
    return u"indigo"_s;
}

ThemeMode Tokens::themeModeFromString(const QString& name) {
    if (name == u"light") return ThemeMode::Light;
    if (name == u"dark") return ThemeMode::Dark;
    return ThemeMode::System;
}

QString Tokens::themeModeToString(ThemeMode mode) {
    switch (mode) {
        case ThemeMode::System:
            return u"system"_s;
        case ThemeMode::Light:
            return u"light"_s;
        case ThemeMode::Dark:
            return u"dark"_s;
    }
    return u"system"_s;
}

}  // namespace JobPrep::Ui::Theme
