#pragma once

#include <QObject>
#include <QString>

namespace JobPrep::Data {
class TrackRepository;
class TopicRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Services {

/// Loads the sample study tracks and topics from `resources/seed/topics.json` (SPEC §9).
/// The operation is idempotent: existing tracks are reused and topics that already
/// exist per track are skipped.
class SeedService : public QObject {
    Q_OBJECT

public:
    SeedService(Data::TrackRepository& tracks, Data::TopicRepository& topics,
                QObject* parent = nullptr);
    ~SeedService() override;

    /// Inserts every missing sample topic in a single transaction.
    bool loadSampleTopics(QString* error = nullptr);

private:
    Data::TrackRepository& m_tracks;
    Data::TopicRepository& m_topics;
};

}  // namespace JobPrep::Services
