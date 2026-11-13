#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTranslator>
#include <QXmlStreamReader>
#include "app/Localization.h"
#include "domain/Structs.h"
#include "services/ReminderService.h"
#include "ui/FormatHelper.h"
#include "ui/NotificationText.h"
#include "PseudoTranslator.h"

using namespace Qt::StringLiterals;
using JobPrep::Services::Notification;
using JobPrep::Services::NotificationKind;
using JobPrep::Ui::FormatHelper;
using JobPrep::Ui::NotificationText;

namespace {

struct TsValidationResult {
    int totalMessages = 0;
    int unfinishedMessages = 0;
    int emptyTranslations = 0;
    int invalidNumerusForms = 0;
    int placeholderMismatches = 0;
    int htmlTagMismatches = 0;
    int acceleratorViolations = 0;
    int leftoverEnglishHelperStrings = 0;
    QStringList errors;
};

QStringList extractTokens(const QString& text, const QRegularExpression& regex) {
    QStringList tokens;
    auto it = regex.globalMatch(text);
    while (it.hasNext()) {
        tokens.append(it.next().captured(0));
    }
    tokens.sort();
    return tokens;
}

void validateTranslationText(const QString& ctx, const QString& src, const QString& trans,
                             const QString& lang, TsValidationResult& result) {
    static const QRegularExpression placeholderRe(u"%[0-9n]"_s);
    static const QRegularExpression htmlTagRe(u"<[^>]+>"_s);

    // 1. Placeholder parity
    const QStringList srcPlaceholders = extractTokens(src, placeholderRe);
    const QStringList transPlaceholders = extractTokens(trans, placeholderRe);
    if (srcPlaceholders != transPlaceholders) {
        result.placeholderMismatches++;
        result.errors.append(QString(u"[%1] Placeholder mismatch in '%2': expected %3, got %4"_s)
                                 .arg(ctx, src, srcPlaceholders.join(u", "_s),
                                      transPlaceholders.join(u", "_s)));
    }

    // 2. HTML tag parity
    const QStringList srcTags = extractTokens(src, htmlTagRe);
    const QStringList transTags = extractTokens(trans, htmlTagRe);
    if (srcTags != transTags) {
        result.htmlTagMismatches++;
        result.errors.append(QString(u"[%1] HTML tag mismatch in '%2': expected %3, got %4"_s)
                                 .arg(ctx, src, srcTags.join(u", "_s), transTags.join(u", "_s)));
    }

    // 3. Option B: No & accelerators in uk and ru translations
    if ((lang == u"uk"_s || lang == u"ru"_s) && trans.contains(u'&')) {
        // Allow literal ampersands if and only if source has literal '&' without accelerator context
        // E.g. "Data & Backups"
        if (!src.contains(u" & "_s)) {
            result.acceleratorViolations++;
            result.errors.append(
                QString(u"[%1] Accelerator '&' forbidden under Option B: '%2' -> '%3'"_s)
                    .arg(ctx, src, trans));
        }
    }

    // 4. Leftover English plural helper strings
    if (ctx.contains(u"FormatHelper"_s) || ctx.contains(u"NotificationText"_s)) {
        if (src == u"1 day"_s || src == u"%1 days"_s || src == u"1 hour"_s ||
            src == u"%1 hours"_s || src == u"1 minute"_s || src == u"%1 minutes"_s) {
            result.leftoverEnglishHelperStrings++;
            result.errors.append(QString(u"[%1] Leftover English-only plural string: '%2'"_s).arg(ctx, src));
        }
    }
}

TsValidationResult validateTsFile(const QString& path, const QString& expectedLang) {
    TsValidationResult result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errors.append(QString(u"Could not open file: %1"_s).arg(path));
        return result;
    }

    QString currentContext;
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == u"TS"_s) {
                const auto lang = xml.attributes().value(u"language"_s).toString();
                if (!expectedLang.isEmpty() && lang != expectedLang) {
                    result.errors.append(QString(u"Expected language '%1' but found '%2'"_s)
                                             .arg(expectedLang, lang));
                }
            } else if (xml.name() == u"name"_s) {
                currentContext = xml.readElementText();
            } else if (xml.name() == u"message"_s) {
                result.totalMessages++;
                const bool isNumerus = (xml.attributes().value(u"numerus"_s) == u"yes"_s);
                QString sourceText;

                // Read within message until </message>
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isEndElement() && xml.name() == u"message"_s) {
                        break;
                    }
                    if (xml.isStartElement() && xml.name() == u"source"_s) {
                        sourceText = xml.readElementText();
                    } else if (xml.isStartElement() && xml.name() == u"translation"_s) {
                        const auto type = xml.attributes().value(u"type"_s).toString();
                        if (type == u"unfinished"_s) {
                            result.unfinishedMessages++;
                        }

                        if (isNumerus) {
                            int numerusCount = 0;
                            while (!xml.atEnd()) {
                                xml.readNext();
                                if (xml.isEndElement() && xml.name() == u"translation"_s) {
                                    break;
                                }
                                if (xml.isStartElement() && xml.name() == u"numerusform"_s) {
                                    numerusCount++;
                                    const QString text = xml.readElementText();
                                    if (type != u"unfinished"_s && text.trimmed().isEmpty()) {
                                        result.emptyTranslations++;
                                    }
                                    if (type != u"unfinished"_s) {
                                        validateTranslationText(currentContext, sourceText, text,
                                                                expectedLang, result);
                                    }
                                }
                            }
                            // uk and ru require exactly 3 plural forms
                            if (expectedLang == u"uk"_s || expectedLang == u"ru"_s) {
                                if (numerusCount != 3) {
                                    result.invalidNumerusForms++;
                                    result.errors.append(
                                        QString(u"Message expected 3 numerusform tags, got %1"_s)
                                            .arg(numerusCount));
                                }
                            }
                        } else {
                            const QString text = xml.readElementText();
                            if (type != u"unfinished"_s && text.trimmed().isEmpty()) {
                                result.emptyTranslations++;
                            }
                            if (type != u"unfinished"_s) {
                                validateTranslationText(currentContext, sourceText, text,
                                                        expectedLang, result);
                            }
                        }
                    }
                }
            }
        }
    }

    if (xml.hasError()) {
        result.errors.append(QString(u"XML error: %1"_s).arg(xml.errorString()));
    }
    return result;
}

}  // namespace

class tst_Translations : public QObject {
    Q_OBJECT

private slots:
    void testTsParsingAndCompletenessFixtures();
    void testRealTsFilesCompleteness();
    void testRealTsFilesSyncWithSource();
    void testPluralMechanismUkrainian();
    void testPluralMechanismRussian();
    void testEmbeddedQmFilesExist();
    void testMissingQtBaseNoWarningNoCrash();
    void testEnglishFallbackAndPseudoTranslator();
    void testRealCatalogsPluralTable();
};

void tst_Translations::testTsParsingAndCompletenessFixtures() {
    const QString fixturesDir = QString::fromUtf8(FIXTURES_DIR);

    // 1. Valid fixture: 0 errors
    const auto validRes = validateTsFile(fixturesDir + u"/valid_uk.ts"_s, u"uk"_s);
    QVERIFY2(validRes.errors.isEmpty(), qPrintable(validRes.errors.join(u", "_s)));
    QCOMPARE(validRes.totalMessages, 2);
    QCOMPARE(validRes.unfinishedMessages, 0);
    QCOMPARE(validRes.emptyTranslations, 0);
    QCOMPARE(validRes.invalidNumerusForms, 0);
    QCOMPARE(validRes.placeholderMismatches, 0);
    QCOMPARE(validRes.htmlTagMismatches, 0);
    QCOMPARE(validRes.acceleratorViolations, 0);
    QCOMPARE(validRes.leftoverEnglishHelperStrings, 0);

    // 2. Broken unfinished fixture: detected
    const auto unfinRes = validateTsFile(fixturesDir + u"/broken_unfinished_uk.ts"_s, u"uk"_s);
    QCOMPARE(unfinRes.totalMessages, 1);
    QCOMPARE(unfinRes.unfinishedMessages, 1);

    // 3. Broken empty translation fixture: detected
    const auto emptyRes = validateTsFile(fixturesDir + u"/broken_empty_uk.ts"_s, u"uk"_s);
    QCOMPARE(emptyRes.totalMessages, 1);
    QCOMPARE(emptyRes.emptyTranslations, 1);

    // 4. Broken numerusform count (2 instead of 3 for uk): detected
    const auto numRes = validateTsFile(fixturesDir + u"/broken_numerus_uk.ts"_s, u"uk"_s);
    QCOMPARE(numRes.totalMessages, 1);
    QCOMPARE(numRes.invalidNumerusForms, 1);

    // 5. Broken placeholder parity (%2 missing): detected
    const auto placeRes = validateTsFile(fixturesDir + u"/broken_placeholders_uk.ts"_s, u"uk"_s);
    QCOMPARE(placeRes.totalMessages, 1);
    QCOMPARE(placeRes.placeholderMismatches, 1);

    // 6. Broken HTML tags (<b> missing): detected
    const auto htmlRes = validateTsFile(fixturesDir + u"/broken_html_uk.ts"_s, u"uk"_s);
    QCOMPARE(htmlRes.totalMessages, 1);
    QCOMPARE(htmlRes.htmlTagMismatches, 1);

    // 7. Broken accelerator under Option B (&Зберегти): detected
    const auto accelRes = validateTsFile(fixturesDir + u"/broken_accelerator_uk.ts"_s, u"uk"_s);
    QCOMPARE(accelRes.totalMessages, 1);
    QCOMPARE(accelRes.acceleratorViolations, 1);

    // 8. Broken leftover English plural helper string: detected
    const auto leftRes = validateTsFile(fixturesDir + u"/broken_leftover_uk.ts"_s, u"uk"_s);
    QCOMPARE(leftRes.totalMessages, 1);
    QCOMPARE(leftRes.leftoverEnglishHelperStrings, 1);
}

void tst_Translations::testRealTsFilesCompleteness() {
    const QString tsDir = QString::fromUtf8(RESOURCES_TRANSLATIONS_DIR);
    const auto ukRes = validateTsFile(tsDir + u"/jobprep_uk.ts"_s, u"uk"_s);
    const auto ruRes = validateTsFile(tsDir + u"/jobprep_ru.ts"_s, u"ru"_s);

    QVERIFY2(ukRes.errors.isEmpty(), qPrintable(ukRes.errors.join(u", "_s)));
    QVERIFY2(ruRes.errors.isEmpty(), qPrintable(ruRes.errors.join(u", "_s)));

    QCOMPARE(ukRes.unfinishedMessages, 0);
    QCOMPARE(ukRes.emptyTranslations, 0);
    QCOMPARE(ukRes.invalidNumerusForms, 0);
    QCOMPARE(ukRes.placeholderMismatches, 0);
    QCOMPARE(ukRes.htmlTagMismatches, 0);
    QCOMPARE(ukRes.acceleratorViolations, 0);
    QCOMPARE(ukRes.leftoverEnglishHelperStrings, 0);

    QCOMPARE(ruRes.unfinishedMessages, 0);
    QCOMPARE(ruRes.emptyTranslations, 0);
    QCOMPARE(ruRes.invalidNumerusForms, 0);
    QCOMPARE(ruRes.placeholderMismatches, 0);
    QCOMPARE(ruRes.htmlTagMismatches, 0);
    QCOMPARE(ruRes.acceleratorViolations, 0);
    QCOMPARE(ruRes.leftoverEnglishHelperStrings, 0);
}

void tst_Translations::testRealTsFilesSyncWithSource() {
    const QString tsDir = QString::fromUtf8(RESOURCES_TRANSLATIONS_DIR);
    const auto ukRes = validateTsFile(tsDir + u"/jobprep_uk.ts"_s, u"uk"_s);
    const auto ruRes = validateTsFile(tsDir + u"/jobprep_ru.ts"_s, u"ru"_s);

    QVERIFY(ukRes.totalMessages >= 500);
    QCOMPARE(ukRes.totalMessages, ruRes.totalMessages);
}

void tst_Translations::testPluralMechanismUkrainian() {
    const QString qmPath = QString::fromUtf8(FIXTURES_BUILD_DIR) + u"/plural_uk.qm"_s;
    QVERIFY2(QFile::exists(qmPath), qPrintable(QString(u"Plural QM file not found: %1"_s).arg(qmPath)));

    QTranslator translator;
    QVERIFY(translator.load(qmPath));
    QCoreApplication::installTranslator(&translator);

    // FormatHelper days: 1440 mins per day
    // CLDR uk: 1, 21, 101 -> form 0 ("день")
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 1), u"1 день"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 21), u"21 день"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 101), u"101 день"_s);

    // CLDR uk: 2, 22 -> form 1 ("дні")
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 2), u"2 дні"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 22), u"22 дні"_s);

    // CLDR uk: 5, 11, 25 -> form 2 ("днів")
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 5), u"5 днів"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 11), u"11 днів"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 25), u"25 днів"_s);

    // NotificationText lead time
    QCOMPARE(NotificationText::formatLeadTime(60 * 1), u"через 1 годину"_s);
    QCOMPARE(NotificationText::formatLeadTime(60 * 2), u"через 2 години"_s);
    QCOMPARE(NotificationText::formatLeadTime(60 * 5), u"через 5 годин"_s);

    QCOMPARE(NotificationText::formatLeadTime(1), u"через 1 хвилину"_s);
    QCOMPARE(NotificationText::formatLeadTime(2), u"через 2 хвилини"_s);
    QCOMPARE(NotificationText::formatLeadTime(5), u"через 5 хвилин"_s);

    // NotificationText overdue summary
    Notification overdueSum;
    overdueSum.kind = NotificationKind::OverdueFollowUps;
    overdueSum.count = 1;
    QCOMPARE(NotificationText::formatMessage(overdueSum), u"1 прострочений фолоу-ап"_s);
    overdueSum.count = 2;
    QCOMPARE(NotificationText::formatMessage(overdueSum), u"2 прострочених фолоу-апи"_s);
    overdueSum.count = 5;
    QCOMPARE(NotificationText::formatMessage(overdueSum), u"5 прострочених фолоу-апів"_s);

    // InAppNotifier "+%n more"
    QCOMPARE(QCoreApplication::translate("JobPrep::Ui::InAppNotifier", "+%n more", "", 1), u"+ще 1"_s);
    QCOMPARE(QCoreApplication::translate("JobPrep::Ui::InAppNotifier", "+%n more", "", 4), u"+ще 4"_s);

    // TrackDialog delete prompt
    const QString tmpl1 = QCoreApplication::translate(
        "JobPrep::Ui::Dialogs::TrackDialog",
        "Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept.",
        "", 1).arg(u"C++"_s);
    QVERIFY(tmpl1.contains(u"1 тему"_s));

    const QString tmpl2 = QCoreApplication::translate(
        "JobPrep::Ui::Dialogs::TrackDialog",
        "Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept.",
        "", 2).arg(u"C++"_s);
    QVERIFY(tmpl2.contains(u"2 теми"_s));

    const QString tmpl5 = QCoreApplication::translate(
        "JobPrep::Ui::Dialogs::TrackDialog",
        "Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept.",
        "", 5).arg(u"C++"_s);
    QVERIFY(tmpl5.contains(u"5 тем"_s));

    QCoreApplication::removeTranslator(&translator);
}

void tst_Translations::testPluralMechanismRussian() {
    const QString qmPath = QString::fromUtf8(FIXTURES_BUILD_DIR) + u"/plural_ru.qm"_s;
    QVERIFY2(QFile::exists(qmPath), qPrintable(QString(u"Plural QM file not found: %1"_s).arg(qmPath)));

    QTranslator translator;
    QVERIFY(translator.load(qmPath));
    QCoreApplication::installTranslator(&translator);

    // FormatHelper days: 1440 mins per day
    // CLDR ru: 1, 21, 101 -> form 0 ("день")
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 1), u"1 день"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 21), u"21 день"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 101), u"101 день"_s);

    // CLDR ru: 2, 22 -> form 1 ("дня")
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 2), u"2 дня"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 22), u"22 дня"_s);

    // CLDR ru: 5, 11, 25 -> form 2 ("дней")
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 5), u"5 дней"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 11), u"11 дней"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 25), u"25 дней"_s);

    // NotificationText lead time
    QCOMPARE(NotificationText::formatLeadTime(60 * 1), u"через 1 час"_s);
    QCOMPARE(NotificationText::formatLeadTime(60 * 2), u"через 2 часа"_s);
    QCOMPARE(NotificationText::formatLeadTime(60 * 5), u"через 5 часов"_s);

    QCOMPARE(NotificationText::formatLeadTime(1), u"через 1 минуту"_s);
    QCOMPARE(NotificationText::formatLeadTime(2), u"через 2 минуты"_s);
    QCOMPARE(NotificationText::formatLeadTime(5), u"через 5 минут"_s);

    // NotificationText overdue summary
    Notification overdueSum;
    overdueSum.kind = NotificationKind::OverdueFollowUps;
    overdueSum.count = 1;
    QCOMPARE(NotificationText::formatMessage(overdueSum), u"1 просроченный фоллоу-ап"_s);
    overdueSum.count = 2;
    QCOMPARE(NotificationText::formatMessage(overdueSum), u"2 просроченных фоллоу-апа"_s);
    overdueSum.count = 5;
    QCOMPARE(NotificationText::formatMessage(overdueSum), u"5 просроченных фоллоу-апов"_s);

    // InAppNotifier "+%n more"
    QCOMPARE(QCoreApplication::translate("JobPrep::Ui::InAppNotifier", "+%n more", "", 1), u"+еще 1"_s);
    QCOMPARE(QCoreApplication::translate("JobPrep::Ui::InAppNotifier", "+%n more", "", 4), u"+еще 4"_s);

    // TrackDialog delete prompt
    const QString tmpl1 = QCoreApplication::translate(
        "JobPrep::Ui::Dialogs::TrackDialog",
        "Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept.",
        "", 1).arg(u"C++"_s);
    QVERIFY(tmpl1.contains(u"1 тему"_s));

    const QString tmpl2 = QCoreApplication::translate(
        "JobPrep::Ui::Dialogs::TrackDialog",
        "Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept.",
        "", 2).arg(u"C++"_s);
    QVERIFY(tmpl2.contains(u"2 темы"_s));

    const QString tmpl5 = QCoreApplication::translate(
        "JobPrep::Ui::Dialogs::TrackDialog",
        "Delete track “%1”? This will delete %n topic(s) and their checklist(s). Logged study time will be kept.",
        "", 5).arg(u"C++"_s);
    QVERIFY(tmpl5.contains(u"5 тем"_s));

    QCoreApplication::removeTranslator(&translator);
}

void tst_Translations::testEmbeddedQmFilesExist() {
    QVERIFY2(QFile::exists(u":/i18n/jobprep_uk.qm"_s), "Embedded resource :/i18n/jobprep_uk.qm not found");
    QVERIFY2(QFile::exists(u":/i18n/jobprep_ru.qm"_s), "Embedded resource :/i18n/jobprep_ru.qm not found");
}

void tst_Translations::testMissingQtBaseNoWarningNoCrash() {
    QTemporaryDir emptyDir;
    QVERIFY(emptyDir.isValid());

    // When pointed to an empty directory, installLanguage must gracefully fail to load qtbase without crashing or warning
    auto res = JobPrep::App::installLanguage(u"uk"_s, emptyDir.path());
    QCOMPARE(res.language, u"uk"_s);
    QVERIFY(res.appTranslatorLoaded);
    QVERIFY(!res.qtBaseTranslatorLoaded);

    JobPrep::App::uninstallLanguage();
}

void tst_Translations::testEnglishFallbackAndPseudoTranslator() {
    // 1. English fallback when no translator is loaded (en)
    JobPrep::App::uninstallLanguage();
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 1), u"1 day"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 2), u"2 days"_s);
    QCOMPARE(FormatHelper::formatActiveProcesses(1), u"1 active process"_s);
    QCOMPARE(FormatHelper::formatActiveProcesses(3), u"3 active processes"_s);
    QCOMPARE(NotificationText::formatLeadTime(60 * 1), u"in 1 hour"_s);
    QCOMPARE(NotificationText::formatLeadTime(60 * 2), u"in 2 hours"_s);

    // 2. Real finished catalog (uk) returns translated Ukrainian plural
    auto resUk = JobPrep::App::installLanguage(u"uk"_s);
    QVERIFY(resUk.appTranslatorLoaded);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 1), u"1 день"_s);
    QCOMPARE(FormatHelper::formatLeadTime(1440 * 2), u"2 дні"_s);
    QCOMPARE(FormatHelper::formatActiveProcesses(1), u"1 активний процес"_s);
    QCOMPARE(FormatHelper::formatActiveProcesses(3), u"3 активні процеси"_s);
    JobPrep::App::uninstallLanguage();

    // 3. Pseudo-translator active: transforms text while preserving formatting
    JobPrep::UiTest::PseudoTranslator pseudo;
    QCoreApplication::installTranslator(&pseudo);

    const QString pseudoResult1 = FormatHelper::formatLeadTime(1440 * 1);
    QVERIFY(!pseudoResult1.isEmpty());
    QVERIFY(pseudoResult1 != u"1 day"_s);
    QVERIFY(pseudoResult1.contains(u'~'));

    QCoreApplication::removeTranslator(&pseudo);
}

void tst_Translations::testRealCatalogsPluralTable() {
    const QList<int> testNumbers = {1, 2, 5, 11, 21, 22, 25, 101};

    // 1. Ukrainian table-driven check
    {
        auto resUk = JobPrep::App::installLanguage(u"uk"_s);
        QVERIFY(resUk.appTranslatorLoaded);

        const QMap<int, QString> expectedDaysUk = {
            {1, u"1 день"_s}, {2, u"2 дні"_s}, {5, u"5 днів"_s}, {11, u"11 днів"_s},
            {21, u"21 день"_s}, {22, u"22 дні"_s}, {25, u"25 днів"_s}, {101, u"101 день"_s}
        };
        const QMap<int, QString> expectedHoursUk = {
            {1, u"1 година"_s}, {2, u"2 години"_s}, {5, u"5 годин"_s}, {11, u"11 годин"_s},
            {21, u"21 година"_s}, {22, u"22 години"_s}, {25, u"25 годин"_s}, {101, u"101 година"_s}
        };
        const QMap<int, QString> expectedMinutesUk = {
            {1, u"1 хвилина"_s}, {2, u"2 хвилини"_s}, {5, u"5 хвилин"_s}, {11, u"11 хвилин"_s},
            {21, u"21 хвилина"_s}, {22, u"22 хвилини"_s}, {25, u"25 хвилин"_s}, {101, u"101 хвилина"_s}
        };
        const QMap<int, QString> expectedActiveProcessesUk = {
            {1, u"1 активний процес"_s}, {2, u"2 активні процеси"_s}, {5, u"5 активних процесів"_s},
            {11, u"11 активних процесів"_s}, {21, u"21 активний процес"_s}, {22, u"22 активні процеси"_s},
            {25, u"25 активних процесів"_s}, {101, u"101 активний процес"_s}
        };
        const QMap<int, QString> expectedNotifLeadHoursUk = {
            {1, u"через 1 годину"_s}, {2, u"через 2 години"_s}, {5, u"через 5 годин"_s},
            {11, u"через 11 годин"_s}, {21, u"через 21 годину"_s}, {22, u"через 22 години"_s},
            {25, u"через 25 годин"_s}, {101, u"через 101 годину"_s}
        };
        const QMap<int, QString> expectedNotifLeadMinutesUk = {
            {1, u"через 1 хвилину"_s}, {2, u"через 2 хвилини"_s}, {5, u"через 5 хвилин"_s},
            {11, u"через 11 хвилин"_s}, {21, u"через 21 хвилину"_s}, {22, u"через 22 хвилини"_s},
            {25, u"через 25 хвилин"_s}, {101, u"через 101 хвилину"_s}
        };
        const QMap<int, QString> expectedOverdueFollowUpsUk = {
            {1, u"1 прострочений фолоу-ап"_s}, {2, u"2 прострочені фолоу-апи"_s}, {5, u"5 прострочених фолоу-апів"_s},
            {11, u"11 прострочених фолоу-апів"_s}, {21, u"21 прострочений фолоу-ап"_s}, {22, u"22 прострочені фолоу-апи"_s},
            {25, u"25 прострочених фолоу-апів"_s}, {101, u"101 прострочений фолоу-ап"_s}
        };
        const QMap<int, QString> expectedApplicationsCountUk = {
            {1, u"1 заявка"_s}, {2, u"2 заявки"_s}, {5, u"5 заявок"_s}, {11, u"11 заявок"_s},
            {21, u"21 заявка"_s}, {22, u"22 заявки"_s}, {25, u"25 заявок"_s}, {101, u"101 заявка"_s}
        };

        for (int n : testNumbers) {
            QCOMPARE(FormatHelper::formatLeadTime(1440 * n), expectedDaysUk.value(n));
            QCOMPARE(FormatHelper::formatLeadTime(60 * n), expectedHoursUk.value(n));
            QCOMPARE(FormatHelper::formatLeadTime(n), expectedMinutesUk.value(n));
            QCOMPARE(FormatHelper::formatActiveProcesses(n), expectedActiveProcessesUk.value(n));
            QCOMPARE(NotificationText::formatLeadTime(60 * n), expectedNotifLeadHoursUk.value(n));
            QCOMPARE(NotificationText::formatLeadTime(n), expectedNotifLeadMinutesUk.value(n));

            Notification overdue;
            overdue.kind = NotificationKind::OverdueFollowUps;
            overdue.count = n;
            QCOMPARE(NotificationText::formatMessage(overdue), expectedOverdueFollowUpsUk.value(n));

            const QString appStr = QCoreApplication::translate("JobPrep::Ui::Pages::JobsPage", "%n application(s)", "", n);
            QCOMPARE(appStr, expectedApplicationsCountUk.value(n));
        }

        JobPrep::App::uninstallLanguage();
    }

    // 2. Russian table-driven check
    {
        auto resRu = JobPrep::App::installLanguage(u"ru"_s);
        QVERIFY(resRu.appTranslatorLoaded);

        const QMap<int, QString> expectedDaysRu = {
            {1, u"1 день"_s}, {2, u"2 дня"_s}, {5, u"5 дней"_s}, {11, u"11 дней"_s},
            {21, u"21 день"_s}, {22, u"22 дня"_s}, {25, u"25 дней"_s}, {101, u"101 день"_s}
        };
        const QMap<int, QString> expectedHoursRu = {
            {1, u"1 час"_s}, {2, u"2 часа"_s}, {5, u"5 часов"_s}, {11, u"11 часов"_s},
            {21, u"21 час"_s}, {22, u"22 часа"_s}, {25, u"25 часов"_s}, {101, u"101 час"_s}
        };
        const QMap<int, QString> expectedMinutesRu = {
            {1, u"1 минута"_s}, {2, u"2 минуты"_s}, {5, u"5 минут"_s}, {11, u"11 минут"_s},
            {21, u"21 минута"_s}, {22, u"22 минуты"_s}, {25, u"25 минут"_s}, {101, u"101 минута"_s}
        };
        const QMap<int, QString> expectedActiveProcessesRu = {
            {1, u"1 активный процесс"_s}, {2, u"2 активных процесса"_s}, {5, u"5 активных процессов"_s},
            {11, u"11 активных процессов"_s}, {21, u"21 активный процесс"_s}, {22, u"22 активных процесса"_s},
            {25, u"25 активных процессов"_s}, {101, u"101 активный процесс"_s}
        };
        const QMap<int, QString> expectedNotifLeadHoursRu = {
            {1, u"через 1 час"_s}, {2, u"через 2 часа"_s}, {5, u"через 5 часов"_s},
            {11, u"через 11 часов"_s}, {21, u"через 21 час"_s}, {22, u"через 22 часа"_s},
            {25, u"через 25 часов"_s}, {101, u"через 101 час"_s}
        };
        const QMap<int, QString> expectedNotifLeadMinutesRu = {
            {1, u"через 1 минуту"_s}, {2, u"через 2 минуты"_s}, {5, u"через 5 минут"_s},
            {11, u"через 11 минут"_s}, {21, u"через 21 минуту"_s}, {22, u"через 22 минуты"_s},
            {25, u"через 25 минут"_s}, {101, u"через 101 минуту"_s}
        };
        const QMap<int, QString> expectedOverdueFollowUpsRu = {
            {1, u"1 просроченный фоллоу-ап"_s}, {2, u"2 просроченных фоллоу-апа"_s}, {5, u"5 просроченных фоллоу-апов"_s},
            {11, u"11 просроченных фоллоу-апов"_s}, {21, u"21 просроченный фоллоу-ап"_s}, {22, u"22 просроченных фоллоу-апа"_s},
            {25, u"25 просроченных фоллоу-апов"_s}, {101, u"101 просроченный фоллоу-ап"_s}
        };
        const QMap<int, QString> expectedApplicationsCountRu = {
            {1, u"1 заявка"_s}, {2, u"2 заявки"_s}, {5, u"5 заявок"_s}, {11, u"11 заявок"_s},
            {21, u"21 заявка"_s}, {22, u"22 заявки"_s}, {25, u"25 заявок"_s}, {101, u"101 заявка"_s}
        };

        for (int n : testNumbers) {
            QCOMPARE(FormatHelper::formatLeadTime(1440 * n), expectedDaysRu.value(n));
            QCOMPARE(FormatHelper::formatLeadTime(60 * n), expectedHoursRu.value(n));
            QCOMPARE(FormatHelper::formatLeadTime(n), expectedMinutesRu.value(n));
            QCOMPARE(FormatHelper::formatActiveProcesses(n), expectedActiveProcessesRu.value(n));
            QCOMPARE(NotificationText::formatLeadTime(60 * n), expectedNotifLeadHoursRu.value(n));
            QCOMPARE(NotificationText::formatLeadTime(n), expectedNotifLeadMinutesRu.value(n));

            Notification overdue;
            overdue.kind = NotificationKind::OverdueFollowUps;
            overdue.count = n;
            QCOMPARE(NotificationText::formatMessage(overdue), expectedOverdueFollowUpsRu.value(n));

            const QString appStr = QCoreApplication::translate("JobPrep::Ui::Pages::JobsPage", "%n application(s)", "", n);
            QCOMPARE(appStr, expectedApplicationsCountRu.value(n));
        }

        JobPrep::App::uninstallLanguage();
    }
}

QTEST_MAIN(tst_Translations)
#include "tst_Translations.moc"
