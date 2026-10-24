#include <QDate>
#include <QSignalSpy>
#include <QTest>
#include "services/DayWatcher.h"

using JobPrep::Services::DayWatcher;

class tst_DayWatcher : public QObject {
    Q_OBJECT

private slots:
    void noSignalOnSameDay();
    void oneSignalWhenDateChanges();
    void noRepeatOnSameChangedDay();
    void dateMovingBackwardsEmitsSignal();
};

void tst_DayWatcher::noSignalOnSameDay() {
    QDate current(2026, 10, 24);
    DayWatcher watcher([&current] { return current; });

    QSignalSpy spy(&watcher, &DayWatcher::dayChanged);
    watcher.checkNow();
    watcher.checkNow();

    QCOMPARE(spy.count(), 0);
}

void tst_DayWatcher::oneSignalWhenDateChanges() {
    QDate current(2026, 10, 24);
    DayWatcher watcher([&current] { return current; });

    QSignalSpy spy(&watcher, &DayWatcher::dayChanged);

    current = QDate(2026, 10, 25);
    watcher.checkNow();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toDate(), QDate(2026, 10, 25));
}

void tst_DayWatcher::noRepeatOnSameChangedDay() {
    QDate current(2026, 10, 24);
    DayWatcher watcher([&current] { return current; });

    QSignalSpy spy(&watcher, &DayWatcher::dayChanged);

    current = QDate(2026, 10, 25);
    watcher.checkNow();
    QCOMPARE(spy.count(), 1);

    // Subsequent check on same changed day
    watcher.checkNow();
    QCOMPARE(spy.count(), 1);
}

void tst_DayWatcher::dateMovingBackwardsEmitsSignal() {
    QDate current(2026, 10, 25);
    DayWatcher watcher([&current] { return current; });

    QSignalSpy spy(&watcher, &DayWatcher::dayChanged);

    // Clock set backwards
    current = QDate(2026, 10, 20);
    watcher.checkNow();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toDate(), QDate(2026, 10, 20));
}

QTEST_GUILESS_MAIN(tst_DayWatcher)
#include "tst_DayWatcher.moc"
