#include "ui/theme/ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include "services/SettingsService.h"
#include "ui/theme/IconProvider.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Theme {

ThemeManager::ThemeManager(Services::SettingsService& settings, QObject* parent)
    : QObject(parent), m_settings(settings) {
    m_mode = Tokens::themeModeFromString(m_settings.themeMode());
    m_accent = Tokens::accentFromString(m_settings.accentPreset());

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this]() {
                if (m_mode == ThemeMode::System) {
                    applyTheme();
                }
            });

    connect(&m_settings, &Services::SettingsService::themeModeChanged, this,
            [this](const QString& modeStr) {
                setThemeMode(Tokens::themeModeFromString(modeStr));
            });

    connect(&m_settings, &Services::SettingsService::accentPresetChanged, this,
            [this](const QString& accentStr) {
                setAccentPreset(Tokens::accentFromString(accentStr));
            });

    applyTheme();
}

ThemeMode ThemeManager::currentMode() const {
    return m_mode;
}

AccentPreset ThemeManager::currentAccent() const {
    return m_accent;
}

bool ThemeManager::isDark() const {
    if (m_mode == ThemeMode::Dark) return true;
    if (m_mode == ThemeMode::Light) return false;
    return detectSystemDark();
}

void ThemeManager::setThemeMode(ThemeMode mode) {
    if (m_mode == mode) return;
    m_mode = mode;
    m_settings.setThemeMode(Tokens::themeModeToString(mode));
    applyTheme();
}

void ThemeManager::setAccentPreset(AccentPreset preset) {
    if (m_accent == preset) return;
    m_accent = preset;
    m_settings.setAccentPreset(Tokens::accentToString(preset));
    applyTheme();
}

ColorTokens ThemeManager::currentTokens() const {
    return Tokens::get(isDark(), m_accent);
}

bool ThemeManager::detectSystemDark() const {
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

void ThemeManager::applyTheme() {
    if (QApplication::style()->objectName().toLower() != u"fusion"_s) {
        QApplication::setStyle(QStyleFactory::create(u"Fusion"_s));
    }

    const ColorTokens tokens = currentTokens();
    QApplication::setPalette(tokens.toPalette());

    QFile file(u":/styles/theme.qss"_s);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString rawQss = QString::fromUtf8(file.readAll());
        file.close();
        const QString processed = processStylesheet(rawQss, tokens);
        qApp->setStyleSheet(processed);
    }

    IconProvider::clearCache();
    emit themeChanged();
}

QString ThemeManager::processStylesheet(const QString& rawQss, const ColorTokens& tokens) {
    QString qss = rawQss;
    qss.replace(u"@bg"_s, tokens.bg.name(QColor::HexRgb));
    qss.replace(u"@surfaceAlt"_s, tokens.surfaceAlt.name(QColor::HexRgb));
    qss.replace(u"@surface"_s, tokens.surface.name(QColor::HexRgb));
    qss.replace(u"@border"_s, tokens.border.name(QColor::HexRgb));
    qss.replace(u"@textMuted"_s, tokens.textMuted.name(QColor::HexRgb));
    qss.replace(u"@text"_s, tokens.text.name(QColor::HexRgb));
    qss.replace(u"@accentHover"_s, tokens.accentHover.name(QColor::HexRgb));
    qss.replace(u"@accentPressed"_s, tokens.accentPressed.name(QColor::HexRgb));
    qss.replace(u"@accentText"_s, tokens.accentText.name(QColor::HexRgb));
    qss.replace(u"@accent"_s, tokens.accent.name(QColor::HexRgb));
    qss.replace(u"@danger"_s, tokens.danger.name(QColor::HexRgb));
    qss.replace(u"@warning"_s, tokens.warning.name(QColor::HexRgb));
    qss.replace(u"@success"_s, tokens.success.name(QColor::HexRgb));
    return qss;
}

}  // namespace JobPrep::Ui::Theme
