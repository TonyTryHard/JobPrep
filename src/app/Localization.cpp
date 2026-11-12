#include "app/Localization.h"

#include <memory>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QTranslator>

using namespace Qt::StringLiterals;

namespace JobPrep::App {

namespace {
std::unique_ptr<QTranslator> s_appTranslator;
std::unique_ptr<QTranslator> s_qtTranslator;
}  // namespace

QString resolveLanguage(const QString& setting, const QStringList& uiLanguages) {
    if (setting == u"en"_s || setting == u"uk"_s || setting == u"ru"_s) {
        return setting;
    }

    for (const QString& tag : uiLanguages) {
        const QString lang = tag.section(QRegularExpression(u"[-_]"_s), 0, 0).toLower();
        if (lang == u"uk"_s) {
            return u"uk"_s;
        }
        if (lang == u"ru"_s) {
            return u"ru"_s;
        }
        if (lang == u"en"_s) {
            return u"en"_s;
        }
    }

    return u"en"_s;
}

void uninstallLanguage() {
    if (s_appTranslator) {
        QCoreApplication::removeTranslator(s_appTranslator.get());
        s_appTranslator.reset();
    }
    if (s_qtTranslator) {
        QCoreApplication::removeTranslator(s_qtTranslator.get());
        s_qtTranslator.reset();
    }
}

InstalledLanguage installLanguage(const QString& language, const QString& customQtTranslationsPath) {
    uninstallLanguage();

    InstalledLanguage result;
    result.language = (language == u"uk"_s || language == u"ru"_s) ? language : u"en"_s;

    if (result.language == u"uk"_s) {
        QLocale::setDefault(QLocale(QLocale::Ukrainian, QLocale::Ukraine));
    } else if (result.language == u"ru"_s) {
        QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));
    } else {
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        return result;
    }

    // Load JobPrep embedded translation from Qt resource system
    s_appTranslator = std::make_unique<QTranslator>();
    const QString qmPath = u":/i18n/jobprep_%1.qm"_s.arg(result.language);
    if (s_appTranslator->load(qmPath)) {
        if (QCoreApplication::installTranslator(s_appTranslator.get())) {
            result.appTranslatorLoaded = true;
        }
    }

    // Load Qt base strings (dialogs, message boxes, line-edit context menus)
    s_qtTranslator = std::make_unique<QTranslator>();
    const QString qtBaseName = u"qtbase_%1"_s.arg(result.language);
    const QString qtTransPath = !customQtTranslationsPath.isEmpty()
                                    ? customQtTranslationsPath
                                    : QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (s_qtTranslator->load(qtBaseName, qtTransPath)) {
        if (QCoreApplication::installTranslator(s_qtTranslator.get())) {
            result.qtBaseTranslatorLoaded = true;
        }
    }

    return result;
}

}  // namespace JobPrep::App
