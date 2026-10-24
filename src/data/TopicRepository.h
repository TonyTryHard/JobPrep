#pragma once

#include <optional>
#include <QHash>
#include <QList>
#include "data/Repository.h"
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// Batched subtask counts (done and total) for a topic.
struct TopicSubtaskProgress {
    int done{0};
    int total{0};
};

/// CRUD for topics and their checklist items (SPEC §5 `topics`, `subtasks`).
/// Topics and subtasks share one `changed()` signal so views reload together.
class TopicRepository : public Repository {

public:
    explicit TopicRepository(Database& database, QObject* parent = nullptr);
    ~TopicRepository() override;

    QList<Domain::Topic> all() const;
    QList<Domain::Topic> byTrack(int trackId) const;
    /// Non-done topics with target_date in [from, to] inclusive.
    QList<Domain::Topic> byTargetDateRange(QDate from, QDate to) const;
    /// Non-done topics with target_date <= upTo.
    QList<Domain::Topic> byTargetDateUpTo(QDate upTo) const;
    std::optional<Domain::Topic> byId(int id) const;

    /// Inserts `topic`, fills its id and timestamps. `resources` is stored as-is.
    bool insert(Domain::Topic& topic);
    bool update(const Domain::Topic& topic);
    bool setStatus(int id, Domain::TopicStatus status);

    /// Rewrites `position` of every topic in `orderedIds` (board reorder).
    bool updatePositions(const QList<int>& orderedIds);

    /// Moves `id` to `targetStatus` and arranges visible topics in the target column
    /// to match `orderedIds`. Topics hidden by a track filter keep their relative order.
    /// Runs atomically in one transaction and emits one changed() signal.
    /// A no-op move writes nothing and emits nothing.
    /// An unknown id returns false, keeps lastError(), and rolls back.
    bool moveTopic(int id, Domain::TopicStatus targetStatus, const QList<int>& orderedIds);

    bool remove(int id);
    int count() const;
    bool isEmpty() const;

    /// Returns subtask progress counts (done and total) for all topics with subtasks
    /// in a single batched query (SPEC §8, A6).
    QHash<int, TopicSubtaskProgress> subtaskProgress() const;

    QList<Domain::Subtask> subtasks(int topicId) const;

    /// Inserts `subtask` and fills its id.
    bool addSubtask(Domain::Subtask& subtask);
    bool updateSubtask(const Domain::Subtask& subtask);
    bool setSubtaskDone(int subtaskId, bool done);
    bool removeSubtask(int subtaskId);

    /// Rewrites `position` of every subtask of `topicId` in `orderedIds`.
    bool reorderSubtasks(int topicId, const QList<int>& orderedIds);

    int subtaskCount(int topicId) const;
    int doneSubtaskCount(int topicId) const;

    QString lastError() const;
    JobPrep::Data::Database& database() const;


private:
    int countSubtasks(int topicId, bool doneOnly) const;
    bool subtaskExists(int subtaskId) const;

    Database& m_database;
};

}  // namespace JobPrep::Data
