#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QTranslator>
#include <QXmlStreamReader>
#include "domain/Structs.h"
#include "services/ReminderService.h"
#include "ui/FormatHelper.h"
#include "ui/NotificationText.h"

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
    QStringList errors;
};

TsValidationResult validateTsFile(const QString& path, const QString& expectedLang) {
    TsValidationResult result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errors.append(QString(u"Could not open file: %1"_s).arg(path));
        return result;
    }

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
            } else if (xml.name() == u"message"_s) {
                result.totalMessages++;
                const bool isNumerus = (xml.attributes().value(u"numerus"_s) == u"yes"_s);

                // Read within message until </message>
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isEndElement() && xml.name() == u"message"_s) {
                        break;
                    }
                    if (xml.isStartElement() && xml.name() == u"translation"_s) {
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
}

void tst_Translations::testRealTsFilesCompleteness() {
    const QString tsDir = QString::fromUtf8(RESOURCES_TRANSLATIONS_DIR);
    const auto ukRes = validateTsFile(tsDir + u"/jobprep_uk.ts"_s, u"uk"_s);
    const auto ruRes = validateTsFile(tsDir + u"/jobprep_ru.ts"_s, u"ru"_s);

    QVERIFY2(ukRes.errors.isEmpty(), qPrintable(ukRes.errors.join(u", "_s)));
    QVERIFY2(ruRes.errors.isEmpty(), qPrintable(ruRes.errors.join(u", "_s)));

    if (qEnvironmentVariableIsEmpty("JOBPREP_REQUIRE_COMPLETE_TRANSLATIONS")) {
        QSKIP(qPrintable(
            QString(u"Part C translation catalogs unfinished by design (%1 unfinished in uk, %2 in ru). "
                    "Set JOBPREP_REQUIRE_COMPLETE_TRANSLATIONS=1 to enforce."_s)
                .arg(ukRes.unfinishedMessages)
                .arg(ruRes.unfinishedMessages)));
    }

    QCOMPARE(ukRes.unfinishedMessages, 0);
    QCOMPARE(ukRes.emptyTranslations, 0);
    QCOMPARE(ukRes.invalidNumerusForms, 0);

    QCOMPARE(ruRes.unfinishedMessages, 0);
    QCOMPARE(ruRes.emptyTranslations, 0);
    QCOMPARE(ruRes.invalidNumerusForms, 0);
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

QTEST_MAIN(tst_Translations)
#include "tst_Translations.moc"
