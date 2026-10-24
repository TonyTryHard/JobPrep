#include <QTemporaryFile>
#include <QSignalSpy>
#include <QTest>
#include <QSettings>
#include "services/SettingsService.h"

using namespace Qt::StringLiterals;
using JobPrep::Services::SettingsService;

class tst_SettingsService : public QObject {
    Q_OBJECT

private slots:
    void defaultLeadMinutesReturns1440And60();
    void sanitizingRemovesNonPositiveDuplicatesAndSortsDescending();
    void emptyOrInvalidStoredValueFallsBackToDefault();
    void signalEmittedOncePerRealChange();
};

void tst_SettingsService::defaultLeadMinutesReturns1440And60() {
    QTemporaryFile temp;
    QVERIFY(temp.open());
    const QString path = temp.fileName();
    temp.close();

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    const QList<int> expected{1440, 60};
    QCOMPARE(svc.interviewLeadMinutes(), expected);
}

void tst_SettingsService::sanitizingRemovesNonPositiveDuplicatesAndSortsDescending() {
    QTemporaryFile temp;
    QVERIFY(temp.open());
    const QString path = temp.fileName();
    temp.close();

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    svc.setInterviewLeadMinutes({60, -10, 1440, 0, 60, 120});

    const QList<int> expected{1440, 120, 60};
    QCOMPARE(svc.interviewLeadMinutes(), expected);
}

void tst_SettingsService::emptyOrInvalidStoredValueFallsBackToDefault() {
    QTemporaryFile temp;
    QVERIFY(temp.open());
    const QString path = temp.fileName();
    temp.close();

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    svc.setInterviewLeadMinutes({-5, 0});
    const QList<int> expected{1440, 60};
    QCOMPARE(svc.interviewLeadMinutes(), expected);
}

void tst_SettingsService::signalEmittedOncePerRealChange() {
    QTemporaryFile temp;
    QVERIFY(temp.open());
    const QString path = temp.fileName();
    temp.close();

    auto qsettings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    SettingsService svc(std::move(qsettings));

    QSignalSpy spy(&svc, &SettingsService::interviewLeadMinutesChanged);

    // 1. First real change: default {1440, 60} -> {120, 30}
    svc.setInterviewLeadMinutes({30, 120});
    QCOMPARE(spy.count(), 1);
    const QList<int> expected{120, 30};
    QCOMPARE(spy.takeFirst().at(0).value<QList<int>>(), expected);

    // 2. Setting equivalent list (unsorted, duplicates) should not emit signal
    svc.setInterviewLeadMinutes({120, 30, 30});
    QCOMPARE(spy.count(), 0);

    // 3. Second real change: {120, 30} -> {60}
    svc.setInterviewLeadMinutes({60});
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).value<QList<int>>(), QList<int>({60}));
}

QTEST_GUILESS_MAIN(tst_SettingsService)
#include "tst_SettingsService.moc"
