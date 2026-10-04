#pragma once

#include <optional>
#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QString>
#include "domain/Enums.h"

namespace JobPrep::Domain {

/// A study track (e.g. C++, English).
struct Track {
    int     id{0};
    QString name;
    QString color;
    QString icon;
    int     position{0};
};

/// A subtask (checklist item) belonging to a topic.
struct Subtask {
    int     id{0};
    int     topicId{0};
    QString text;
    bool    done{false};
    int     position{0};
};

/// A study topic within a track.
struct Topic {
    int                    id{0};
    int                    trackId{0};
    QString                title;
    TopicStatus            status{TopicStatus::Backlog};
    Priority               priority{Priority::Normal};
    std::optional<QDate>   targetDate;
    QString                notes;
    QString                resources;  ///< Raw JSON: [{"title":"...","url":"..."}]
    QString                tags;
    int                    position{0};
    QDateTime              createdAt;
    QDateTime              updatedAt;
};

/// A logged study session (may not be tied to a specific topic).
struct StudySession {
    int                  id{0};
    std::optional<int>   topicId;
    QDate                sessionDate;
    int                  minutes{0};
    QString              note;
};

/// A job application entry.
struct JobApplication {
    int                            id{0};
    QString                        company;
    QString                        position;
    QString                        url;
    QString                        source;
    QString                        resumeVersion;
    QString                        location;
    WorkMode                       workMode{WorkMode::Remote};
    std::optional<int>             salaryMin;
    std::optional<int>             salaryMax;
    QString                        currency;
    ApplicationStatus              status{ApplicationStatus::Applied};
    std::optional<QDate>           appliedDate;
    QString                        nextAction;
    std::optional<QDate>           nextActionDate;
    QString                        contactName;
    QString                        contactEmail;
    QString                        notes;
    QDateTime                      createdAt;
    QDateTime                      updatedAt;
};

/// A record of a status transition for a job application.
struct StatusChange {
    int                               id{0};
    int                               applicationId{0};
    std::optional<ApplicationStatus>  fromStatus;
    ApplicationStatus                 toStatus{ApplicationStatus::Applied};
    QDateTime                         changedAt;
    QString                           note;
};

/// An interview event linked to a job application.
struct Interview {
    int              id{0};
    int              applicationId{0};
    QDateTime        startAt;
    int              durationMin{60};
    InterviewType    type{InterviewType::Technical};
    QString          place;
    QString          interviewer;
    QString          notes;
    InterviewOutcome outcome{InterviewOutcome::Pending};
};

/// A merged calendar/agenda item produced by AgendaService.
struct AgendaItem {
    AgendaKind            kind{AgendaKind::Interview};
    QDate                 date;
    std::optional<QTime>  time;
    QString               title;
    QString               subtitle;
    int                   refId{0};
    QColor                color;
};

}  // namespace JobPrep::Domain
