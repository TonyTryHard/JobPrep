#pragma once

#include <functional>
#include <QDate>
#include <QDateTime>
#include <QList>
#include "domain/Structs.h"

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {

/// Merges interviews, follow-ups and study targets into a flat, sorted list
/// of AgendaItem values for a date range (SPEC §3.4 C6).
class AgendaService {
public:
    using Clock = std::function<QDateTime()>;

    /// `tracks` is required: it provides the track name used as the study-target subtitle.
    explicit AgendaService(Data::InterviewRepository& interviews,
                           Data::ApplicationRepository& apps,
                           Data::TopicRepository& topics,
                           Data::TrackRepository& tracks,
                           Clock clock = [] { return QDateTime::currentDateTime(); });

    void setClock(Clock clock);

    /// Returns items whose date falls in [from, to] inclusive, sorted by
    /// (date, kind, time, title). kind order: Interview < FollowUp < StudyTarget.
    QList<Domain::AgendaItem> items(QDate from, QDate to) const;

private:
    Data::InterviewRepository& m_interviews;
    Data::ApplicationRepository& m_apps;
    Data::TopicRepository& m_topics;
    Data::TrackRepository& m_tracks;
    Clock m_clock;
};

}  // namespace JobPrep::Services
