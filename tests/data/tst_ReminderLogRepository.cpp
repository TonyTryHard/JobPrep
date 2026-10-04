#include <QDateTime>
#include <QSignalSpy>
#include <QTest>
#include <QTime>
#include "TestDatabase.h"
#include "data/ReminderLogRepository.h"

using namespace Qt::StringLiterals;
using JobPrep::Data::ReminderLogRepository;
using JobPrep::Tests::TestDatabase;

class tst_ReminderLogRepository : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void keyIsRememberedWithTimestamp();
    void markSentTwiceKeepsFirstTimestamp();
    void removeForgetsKey();
    void sentKeysListsEveryKey();
    void emptyKeyIsRejected();

private:
    std::unique_ptr<TestDatabase> m_db;
    std::unique_ptr<ReminderLogRepository> m_log;
};

void tst_ReminderLogRepository::init() {
    m_db = std::make_unique<TestDatabase>(u"tst_reminders"_s);
    QVERIFY(m_db->open(u":memory:"_s));
    m_log = std::make_unique<ReminderLogRepository>(m_db->db());
}

void tst_ReminderLogRepository::cleanup() {
    m_log.reset();
    m_db.reset();
}

void tst_ReminderLogRepository::keyIsRememberedWithTimestamp() {
    QVERIFY(!m_log->wasSent(u"interview:7:1440"_s));

    QSignalSpy spy(m_log.get(), &ReminderLogRepository::changed);
    const QDateTime sentAt(QDate(2026, 10, 1), QTime(9, 30));
    QVERIFY(m_log->markSent(u"interview:7:1440"_s, sentAt));
    QCOMPARE(spy.count(), 1);

    QVERIFY(m_log->wasSent(u"interview:7:1440"_s));
    QVERIFY(!m_log->wasSent(u"interview:7:60"_s));
    QCOMPARE(m_log->sentKeys(), QList<QString>({u"interview:7:1440"_s}));
    QCOMPARE(m_db->scalar(u"SELECT sent_at FROM reminders_sent WHERE key='interview:7:1440'"_s)
                 .toString(),
             u"2026-10-01T09:30:00"_s);

    // The default timestamp is the current time, truncated to seconds.
    QVERIFY(m_log->markSent(u"followup:3:2026-10-05"_s));
    QCOMPARE(m_db->scalar(u"SELECT sent_at FROM reminders_sent WHERE key='followup:3:2026-10-05'"_s)
                 .toString()
                 .size(),
             19);
}

void tst_ReminderLogRepository::markSentTwiceKeepsFirstTimestamp() {
    const QDateTime first(QDate(2026, 10, 1), QTime(9, 30));
    const QDateTime second(QDate(2026, 10, 2), QTime(9, 30));
    QVERIFY(m_log->markSent(u"overdue:3:2026-10-05"_s, first));
    QVERIFY(m_log->markSent(u"overdue:3:2026-10-05"_s, second));

    QCOMPARE(m_log->sentKeys().size(), 1);
    QCOMPARE(m_db->countRows(u"reminders_sent"_s), 1);
    QCOMPARE(m_db->scalar(u"SELECT sent_at FROM reminders_sent"_s).toString(),
             u"2026-10-01T09:30:00"_s);
}

void tst_ReminderLogRepository::removeForgetsKey() {
    QVERIFY(m_log->markSent(u"interview:1:60"_s));

    QSignalSpy spy(m_log.get(), &ReminderLogRepository::changed);
    QVERIFY(m_log->remove(u"interview:1:60"_s));
    QCOMPARE(spy.count(), 1);
    QVERIFY(!m_log->wasSent(u"interview:1:60"_s));
    QVERIFY(!m_log->remove(u"interview:1:60"_s));
    QCOMPARE(spy.count(), 1);
}

void tst_ReminderLogRepository::sentKeysListsEveryKey() {
    QVERIFY(m_log->markSent(u"interview:2:1440"_s, QDateTime(QDate(2026, 10, 3), QTime(8, 0))));
    QVERIFY(m_log->markSent(u"interview:1:60"_s, QDateTime(QDate(2026, 10, 1), QTime(8, 0))));
    QVERIFY(m_log->markSent(u"followup:4:2026-10-06"_s, QDateTime(QDate(2026, 10, 2), QTime(8, 0))));

    const QList<QString> keys = m_log->sentKeys();
    QCOMPARE(keys.size(), 3);
    QCOMPARE(keys.at(0), u"interview:1:60"_s);
    QCOMPARE(keys.at(1), u"followup:4:2026-10-06"_s);
    QCOMPARE(keys.at(2), u"interview:2:1440"_s);
}

void tst_ReminderLogRepository::emptyKeyIsRejected() {
    QSignalSpy spy(m_log.get(), &ReminderLogRepository::changed);
    QVERIFY(!m_log->markSent(QString()));
    QVERIFY(!m_log->lastError().isEmpty());
    QVERIFY(!m_log->wasSent(QString()));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(m_db->countRows(u"reminders_sent"_s), 0);
}

QTEST_GUILESS_MAIN(tst_ReminderLogRepository)
#include "tst_ReminderLogRepository.moc"
