#pragma once

#include <QString>
#include <QStringList>

namespace JobPrep::App {

/// Resolves effective UI language ("en", "uk", "ru") from user setting and system UI languages.
/// Explicit "en", "uk", or "ru" settings are honored directly.
/// If setting is "system" (or empty/unrecognized), uiLanguages is inspected in order;
/// the first supported language ("uk", "ru", "en") wins. If none match or list is empty, returns "en".
QString resolveLanguage(const QString& setting, const QStringList& uiLanguages);

/// Result of installing language translators.
struct InstalledLanguage {
    QString language;
    bool appTranslatorLoaded{false};
    bool qtBaseTranslatorLoaded{false};
};

/// Installs application and Qt base translators for the specified language ("en", "uk", "ru").
/// Sets QLocale::setDefault to the corresponding locale.
/// Repeated calls safely remove previously installed translators before installing new ones.
InstalledLanguage installLanguage(const QString& language, const QString& customQtTranslationsPath = QString());

/// Uninstalls and resets all installed translators, restoring default English state.
void uninstallLanguage();

}  // namespace JobPrep::App
