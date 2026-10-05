#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QStringList>
#include "services/SeedService.h"
#include "data/Database.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/EnumStrings.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

struct SeedTopic {
    QString          title;
    Domain::Priority priority{Domain::Priority::Normal};
    QString          tags;
    QStringList      subtasks;
};

struct SeedTrack {
    QString      name;
    QString      color;
    QString      icon;
    QList<SeedTopic> topics;
};

/// Reads the bundled sample data; `error` describes the first problem found.
bool readSeedFile(QList<SeedTrack>* tracks, QString* error) {
    QFile file(u":/seed/topics.json"_s);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = u"Cannot read seed data: %1"_s.arg(file.errorString());
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = u"Invalid seed data: %1"_s.arg(parseError.errorString());
        return false;
    }

    const QJsonArray trackArray = document.object().value(u"tracks"_s).toArray();
    if (trackArray.isEmpty()) {
        if (error) *error = u"Invalid seed data: no tracks"_s;
        return false;
    }

    for (const QJsonValue& trackValue : trackArray) {
        const QJsonObject trackObject = trackValue.toObject();
        SeedTrack track;
        track.name = trackObject.value(u"name"_s).toString();
        track.color = trackObject.value(u"color"_s).toString();
        track.icon = trackObject.value(u"icon"_s).toString();
        if (track.name.isEmpty() || track.color.isEmpty()) {
            if (error) *error = u"Invalid seed data: a track has no name or color"_s;
            return false;
        }

        for (const QJsonValue& topicValue : trackObject.value(u"topics"_s).toArray()) {
            const QJsonObject topicObject = topicValue.toObject();
            SeedTopic seedTopic;
            seedTopic.title = topicObject.value(u"title"_s).toString();
            seedTopic.tags = topicObject.value(u"tags"_s).toString();
            seedTopic.priority = Domain::EnumStrings::priorityFromString(
                                     topicObject.value(u"priority"_s).toString())
                                     .value_or(Domain::Priority::Normal);
            for (const QJsonValue& subtask : topicObject.value(u"subtasks"_s).toArray()) {
                seedTopic.subtasks.append(subtask.toString());
            }
            if (seedTopic.title.isEmpty() || seedTopic.subtasks.isEmpty()) {
                if (error) *error = u"Invalid seed data: a topic has no title or checklist"_s;
                return false;
            }
            track.topics.append(seedTopic);
        }
        tracks->append(track);
    }
    return true;
}

/// A poisoned or rejected unit of work may leave no SQL error behind, but the caller
/// still needs something to show.
QString seedFailure(const QString& lastError) {
    return lastError.isEmpty() ? u"The seed was rolled back."_s : lastError;
}

}  // namespace

SeedService::SeedService(Data::TrackRepository& tracks, Data::TopicRepository& topics,
                         QObject* parent)
    : QObject(parent), m_tracks(tracks), m_topics(topics) {}

SeedService::~SeedService() = default;

bool SeedService::loadSampleTopics(QString* error) {
    QList<SeedTrack> seedTracks;
    if (!readSeedFile(&seedTracks, error)) return false;

    auto transaction = m_tracks.database().transaction();
    if (!transaction.isActive()) {
        if (error) *error = m_tracks.lastError();
        return false;
    }

    // The whole seed is one unit of work. The repositories defer their changed() to the
    // outer commit, so consumers see exactly one signal per repository, after the data.
    int position = 0;
    for (const SeedTrack& seedTrack : seedTracks) {
        int trackId = 0;
        if (const auto existing = m_tracks.idByName(seedTrack.name)) {
            trackId = *existing;
        } else {
            Domain::Track track;
            track.name = seedTrack.name;
            track.color = seedTrack.color;
            track.icon = seedTrack.icon;
            track.position = position;
            if (!m_tracks.insert(track)) {
                if (error) *error = m_tracks.lastError();
                return false;
            }
            trackId = track.id;
        }
        ++position;

        QSet<QString> existingTitles;
        for (const Domain::Topic& topic : m_topics.byTrack(trackId)) {
            existingTitles.insert(topic.title);
        }

        int topicPosition = 0;
        for (const SeedTopic& seedTopic : seedTrack.topics) {
            if (existingTitles.contains(seedTopic.title)) {
                ++topicPosition;
                continue;
            }
            Domain::Topic topic;
            topic.trackId = trackId;
            topic.title = seedTopic.title;
            topic.status = Domain::TopicStatus::Backlog;
            topic.priority = seedTopic.priority;
            topic.tags = seedTopic.tags;
            topic.position = topicPosition;
            if (!m_topics.insert(topic)) {
                if (error) *error = m_topics.lastError();
                return false;
            }

            int subtaskPosition = 0;
            for (const QString& text : seedTopic.subtasks) {
                Domain::Subtask subtask;
                subtask.topicId = topic.id;
                subtask.text = text;
                subtask.position = subtaskPosition;
                if (!m_topics.addSubtask(subtask)) {
                    if (error) *error = m_topics.lastError();
                    return false;
                }
                ++subtaskPosition;
            }
            ++topicPosition;
        }
    }

    if (!transaction.commit()) {
        if (error) *error = seedFailure(m_tracks.lastError());
        return false;
    }
    return true;
}

}  // namespace JobPrep::Services
