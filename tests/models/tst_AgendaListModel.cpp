#include <QtTest>
#include "domain/Enums.h"
#include "domain/Structs.h"
#include "models/AgendaListModel.h"
#include "ui/theme/AgendaKindStyle.h"

using namespace Qt::StringLiterals;

namespace {

using namespace JobPrep;

Domain::AgendaItem interviewItem() {
    Domain::AgendaItem item;
    item.kind = Domain::AgendaKind::Interview;
    item.date = QDate(2026, 10, 12);
    item.time = QTime(10, 30);
    item.title = u"Google L6 phone screen"_s;
    item.subtitle = u"Acme, Phone call"_s;
    item.refId = 7;
    item.past = false;
    return item;
}

}  // namespace

class tst_AgendaListModel : public QObject {
    Q_OBJECT

private slots:
    void emptyModel();
    void rolesForInterviewItem();
    void rolesForFollowUpAndTarget();
    void pastFlag();
    void agendaKindStyle();
};

void tst_AgendaListModel::emptyModel() {
    JobPrep::Models::AgendaListModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.items().size(), 0);
}

void tst_AgendaListModel::rolesForInterviewItem() {
    JobPrep::Models::AgendaListModel model;
    auto item = interviewItem();
    model.setItems({item});

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0, 0);

    QCOMPARE(index.data(Qt::DisplayRole).toString(), u"Google L6 phone screen"_s);
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::KindRole).toInt(),
             static_cast<int>(JobPrep::Domain::AgendaKind::Interview));
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::IdRole).toInt(), 7);
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::SubtitleRole).toString(),
             u"Acme, Phone call"_s);
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::TimeRole).toTime(), QTime(10, 30));
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::DateRole).toDate(), QDate(2026, 10, 12));
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::PastRole).toBool(), false);
    // Tooltips explain entries that cannot open a dialog in this milestone; interviews
    // have a real handler so they carry none (Amendment C6).
    QCOMPARE(index.data(Qt::ToolTipRole).toString(), u""_s);

    // No color/icon information ever leaks into the model (Amendment C5).
    const QHash<int, QByteArray> roles = model.roleNames();
    QVERIFY(!roles.contains(Qt::ForegroundRole));
    QVERIFY(!roles.contains(Qt::DecorationRole));
    QVERIFY(!roles.contains(Qt::BackgroundRole));
}

void tst_AgendaListModel::rolesForFollowUpAndTarget() {
    JobPrep::Models::AgendaListModel model;

    JobPrep::Domain::AgendaItem followUp;
    followUp.kind = JobPrep::Domain::AgendaKind::FollowUp;
    followUp.date = QDate(2026, 10, 13);
    followUp.title = u"Send thank-you email"_s;
    followUp.subtitle = u"Acme"_s;

    JobPrep::Domain::AgendaItem target;
    target.kind = JobPrep::Domain::AgendaKind::StudyTarget;
    target.date = QDate(2026, 10, 14);
    target.title = u"Review STL containers"_s;
    target.subtitle = u"English/Interviews"_s;

    model.setItems({followUp, target});
    QCOMPARE(model.rowCount(), 2);

    QModelIndex index = model.index(0, 0);
    // Not time-bound: follow-ups and targets have no clock time (empty time text).
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::TimeRole).toString(), u""_s);
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::KindRole).toInt(),
             static_cast<int>(JobPrep::Domain::AgendaKind::FollowUp));

    index = model.index(1, 0);
    QCOMPARE(index.data(Qt::DisplayRole).toString(), u"Review STL containers"_s);
    QCOMPARE(index.data(JobPrep::Models::AgendaListModel::DateRole).toDate(), QDate(2026, 10, 14));
}

void tst_AgendaListModel::pastFlag() {
    JobPrep::Models::AgendaListModel model;

    JobPrep::Domain::AgendaItem past;
    past.kind = JobPrep::Domain::AgendaKind::Interview;
    past.date = QDate(2026, 10, 1);
    past.title = u"Finished interview"_s;
    past.past = true;

    model.setItems({past});
    QCOMPARE(model.index(0, 0).data(JobPrep::Models::AgendaListModel::PastRole).toBool(), true);
}

void tst_AgendaListModel::agendaKindStyle() {
    using JobPrep::Domain::AgendaKind;
    using JobPrep::Ui::Theme::AgendaKindStyle;

    const QColor interview = AgendaKindStyle::color(AgendaKind::Interview);
    const QColor followUp = AgendaKindStyle::color(AgendaKind::FollowUp);
    const QColor target = AgendaKindStyle::color(AgendaKind::StudyTarget);
    QVERIFY(interview.isValid());
    QVERIFY(followUp.isValid());
    QVERIFY(target.isValid());
    QVERIFY(interview != followUp);
    QVERIFY(followUp != target);

    const QString interviewLabel = AgendaKindStyle::label(AgendaKind::Interview);
    QVERIFY(!interviewLabel.isEmpty());
    QVERIFY(AgendaKindStyle::label(AgendaKind::FollowUp) != interviewLabel);
    QVERIFY(AgendaKindStyle::label(AgendaKind::StudyTarget) != interviewLabel);

    QVERIFY(AgendaKindStyle::iconName(AgendaKind::Interview).startsWith(u"agenda-"_s));
    QVERIFY(AgendaKindStyle::iconName(AgendaKind::FollowUp).startsWith(u"agenda-"_s));
    QVERIFY(AgendaKindStyle::iconName(AgendaKind::StudyTarget).startsWith(u"agenda-"_s));

    const QIcon icon = AgendaKindStyle::icon(AgendaKind::Interview, interview);
    QVERIFY(!icon.isNull());
}

QTEST_MAIN(tst_AgendaListModel)
#include "tst_AgendaListModel.moc"