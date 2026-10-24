#include "services/AgendaService.h"

#include <algorithm>
#include <QHash>
#include "data/ApplicationRepository.h"
#include "data/InterviewRepository.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/EnumStrings.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

QString interviewTitle(const Domain::Interview& iv, const QHash<int, Domain::JobApplication>& apps) {
    const QString typeName = Domain::EnumStrings::toString(iv.type);
    // Capitalize first letter of type
    QString prettyType = typeName;
    if (!prettyType.isEmpty()) {
        prettyType[0] = prettyType[0].toUpper();
    }
    const auto it = apps.constFind(iv.applicationId);
    if (it != apps.constEnd()) {
        return u"%1 · %2"_s.arg(prettyType, it->company);
    }
    return prettyType;
}

QString interviewSubtitle(const Domain::Interview& iv) {
    if (!iv.place.isEmpty() && !iv.interviewer.isEmpty()) {
        return u"%1 · %2"_s.arg(iv.place, iv.interviewer);
    }
    if (!iv.place.isEmpty()) return iv.place;
    return iv.interviewer;
}

}  // namespace

AgendaService::AgendaService(Data::InterviewRepository& interviews,
                             Data::ApplicationRepository& apps,
                             Data::TopicRepository& topics,
                             Data::TrackRepository& tracks,
                             Clock clock)
    : m_interviews(interviews),
      m_apps(apps),
      m_topics(topics),
      m_tracks(tracks),
      m_clock(clock ? std::move(clock) : Clock([] { return QDateTime::currentDateTime(); })) {}

void AgendaService::setClock(Clock clock) {
    m_clock = clock ? std::move(clock) : Clock([] { return QDateTime::currentDateTime(); });
}

QList<Domain::AgendaItem> AgendaService::items(QDate from, QDate to) const {
    QList<Domain::AgendaItem> result;
    if (!from.isValid() || !to.isValid() || from > to) {
        return result;
    }

    const QDateTime now = m_clock();
    const QDate today = now.date();

    // Cache applications by id for title lookup (interviews)
    const auto allApps = m_apps.all();
    QHash<int, Domain::JobApplication> appsById;
    for (const auto& app : allApps) {
        appsById.insert(app.id, app);
    }

    // Cache tracks by id for study target subtitle
    QHash<int, QString> trackNamesById;
    const auto allTracks = m_tracks.all();
    for (const auto& track : allTracks) {
        trackNamesById.insert(track.id, track.name);
    }

    // 1. Interviews: half-open day bounds [from 00:00:00, to+1 00:00:00).
    // In terms of closed-range QDateTime, the end of the day is 23:59:59.
    const QDateTime ivFrom(from, QTime(0, 0, 0));
    const QDateTime ivTo(to, QTime(23, 59, 59));
    const auto interviews = m_interviews.inRange(ivFrom, ivTo);

    for (const auto& iv : interviews) {
        if (iv.outcome == Domain::InterviewOutcome::Cancelled) {
            continue;
        }
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::Interview;
        item.date = iv.startAt.date();
        item.time = iv.startAt.time();
        item.title = interviewTitle(iv, appsById);
        item.subtitle = interviewSubtitle(iv);
        item.refId = iv.id;
        item.past = (iv.startAt < now);
        result.append(item);
    }

    // 2. Follow-ups: inclusive range [from, to] on applications with next_action_date
    const auto followUpApps = m_apps.byNextActionDateRange(from, to);
    for (const auto& app : followUpApps) {
        if (!app.nextActionDate.has_value()) {
            continue;
        }
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::FollowUp;
        item.date = *app.nextActionDate;
        item.time = std::nullopt;
        item.title = u"Follow-up · %1"_s.arg(app.company);
        item.subtitle = app.nextAction;
        item.refId = app.id;
        item.past = (item.date < today);
        result.append(item);
    }

    // 3. Study targets: inclusive range [from, to] on topics with target_date
    const auto targetTopics = m_topics.byTargetDateRange(from, to);
    for (const auto& topic : targetTopics) {
        if (!topic.targetDate.has_value()) {
            continue;
        }
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::StudyTarget;
        item.date = *topic.targetDate;
        item.time = std::nullopt;
        item.title = u"Target · %1"_s.arg(topic.title);
        item.subtitle = trackNamesById.value(topic.trackId);
        item.refId = topic.id;
        item.past = (item.date < today);
        result.append(item);
    }

    // Deterministic sort:
    // (date, kind, time, title)
    // kind order: Interview < FollowUp < StudyTarget
    std::sort(result.begin(), result.end(), [](const Domain::AgendaItem& a, const Domain::AgendaItem& b) {
        if (a.date != b.date) {
            return a.date < b.date;
        }
        if (a.kind != b.kind) {
            return static_cast<int>(a.kind) < static_cast<int>(b.kind);
        }
        if (a.time.has_value() != b.time.has_value()) {
            return a.time.has_value(); // with time sorts first
        }
        if (a.time.has_value() && a.time != b.time) {
            return *a.time < *b.time;
        }
        return a.title < b.title;
    });

    return result;
}

QList<Domain::AgendaItem> AgendaService::todayItems(QDate today) const {
    QList<Domain::AgendaItem> result;
    if (!today.isValid()) return result;

    const QDateTime now = m_clock();

    // Cache applications by id for title lookup (interviews)
    const auto allApps = m_apps.all();
    QHash<int, Domain::JobApplication> appsById;
    for (const auto& app : allApps) {
        appsById.insert(app.id, app);
    }

    // Cache tracks by id for study target subtitle
    QHash<int, QString> trackNamesById;
    const auto allTracks = m_tracks.all();
    for (const auto& track : allTracks) {
        trackNamesById.insert(track.id, track.name);
    }

    // 1. Interviews on `today` (including earlier today, past set when startAt < now), excluding cancelled
    const QDateTime ivFrom(today, QTime(0, 0, 0));
    const QDateTime ivTo(today, QTime(23, 59, 59));
    const auto interviews = m_interviews.inRange(ivFrom, ivTo);
    for (const auto& iv : interviews) {
        if (iv.outcome == Domain::InterviewOutcome::Cancelled) {
            continue;
        }
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::Interview;
        item.date = iv.startAt.date();
        item.time = iv.startAt.time();
        item.title = interviewTitle(iv, appsById);
        item.subtitle = interviewSubtitle(iv);
        item.refId = iv.id;
        item.past = (iv.startAt < now);
        result.append(item);
    }

    // 2. Follow-ups with next_action_date <= today for non-closed applications
    const auto followUps = m_apps.byNextActionDateUpTo(today);
    for (const auto& app : followUps) {
        if (!app.nextActionDate.has_value()) continue;
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::FollowUp;
        item.date = *app.nextActionDate;
        item.time = std::nullopt;
        item.title = u"Follow-up · %1"_s.arg(app.company);
        item.subtitle = app.nextAction;
        item.refId = app.id;
        item.past = (item.date < today);
        result.append(item);
    }

    // 3. Topics with target_date <= today and status != Done
    const auto targetTopics = m_topics.byTargetDateUpTo(today);
    for (const auto& topic : targetTopics) {
        if (!topic.targetDate.has_value()) continue;
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::StudyTarget;
        item.date = *topic.targetDate;
        item.time = std::nullopt;
        item.title = u"Target · %1"_s.arg(topic.title);
        item.subtitle = trackNamesById.value(topic.trackId);
        item.refId = topic.id;
        item.past = (item.date < today);
        result.append(item);
    }

    // Order per A4:
    // Overdue follow-ups and overdue topics first (oldest date first, then kind FollowUp < StudyTarget, then title),
    // then today's items by time (timed items first sorted by time, non-timed sorted by Interview < FollowUp < StudyTarget, then title).
    std::sort(result.begin(), result.end(), [today](const Domain::AgendaItem& a, const Domain::AgendaItem& b) {
        const bool aOverdue = (a.date < today);
        const bool bOverdue = (b.date < today);

        if (aOverdue != bOverdue) {
            return aOverdue; // Overdue items come first
        }

        if (aOverdue && bOverdue) {
            if (a.date != b.date) {
                return a.date < b.date; // Oldest date first
            }
            if (a.kind != b.kind) {
                return static_cast<int>(a.kind) < static_cast<int>(b.kind);
            }
            return a.title < b.title;
        }

        // Both are today (a.date == today && b.date == today)
        if (a.time.has_value() != b.time.has_value()) {
            return a.time.has_value(); // Items with time sort first
        }
        if (a.time.has_value() && a.time != b.time) {
            return *a.time < *b.time;
        }
        if (a.kind != b.kind) {
            return static_cast<int>(a.kind) < static_cast<int>(b.kind);
        }
        return a.title < b.title;
    });

    return result;
}

QList<Domain::AgendaItem> AgendaService::upcomingInterviews(const QDateTime& from, int limit) const {
    QList<Domain::AgendaItem> result;
    if (!from.isValid() || limit <= 0) return result;

    const QDateTime now = m_clock();

    const auto allApps = m_apps.all();
    QHash<int, Domain::JobApplication> appsById;
    for (const auto& app : allApps) {
        appsById.insert(app.id, app);
    }

    const auto interviews = m_interviews.upcoming(from, limit);
    for (const auto& iv : interviews) {
        Domain::AgendaItem item;
        item.kind = Domain::AgendaKind::Interview;
        item.date = iv.startAt.date();
        item.time = iv.startAt.time();
        item.title = interviewTitle(iv, appsById);
        item.subtitle = interviewSubtitle(iv);
        item.refId = iv.id;
        item.past = (iv.startAt < now);
        result.append(item);
    }
    return result;
}

}  // namespace JobPrep::Services
