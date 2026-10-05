#pragma once

#include <optional>
#include <QList>
#include "data/Repository.h"
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// CRUD for topics and their checklist items (SPEC §5 `topics`, `subtasks`).
/// Topics and subtasks share one `changed()` signal so views reload together.
class TopicRepository : public Repository {

public:
    explicit TopicRepository(Database& database, QObject* parent = nullptr);
    ~TopicRepository() override;

    QList<Domain::Topic> all() const;
    QList<Domain::Topic> byTrack(int trackId) const;
    std::optional<Domain::Topic> byId(int id) const;

    /// Inserts `topic`, fills its id and timestamps. `resources` is stored as-is.
    bool insert(Domain::Topic& topic);
    bool update(const Domain::Topic& topic);
    bool setStatus(int id, Domain::TopicStatus status);

    /// Rewrites `position` of every topic in `orderedIds` (board reorder).
    bool updatePositions(const QList<int>& orderedIds);

    bool remove(int id);
    int count() const;
    bool isEmpty() const;

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
