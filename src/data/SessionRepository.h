#pragma once

#include <optional>
#include <QList>
#include "data/Repository.h"
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// CRUD for logged study sessions (SPEC §5 `study_sessions`).
/// Sessions survive a deleted topic: their `topic_id` becomes NULL.
class SessionRepository : public Repository {

public:
    explicit SessionRepository(Database& database, QObject* parent = nullptr);
    ~SessionRepository() override;

    QList<Domain::StudySession> all() const;
    QList<Domain::StudySession> byTopic(int topicId) const;

    /// Sessions whose date is within [from, to], inclusive.
    QList<Domain::StudySession> inRange(QDate from, QDate to) const;

    std::optional<Domain::StudySession> byId(int id) const;

    /// Inserts `session` and fills its id.
    bool insert(Domain::StudySession& session);
    bool update(const Domain::StudySession& session);
    bool remove(int id);

    int count() const;
    int totalMinutesInRange(QDate from, QDate to) const;
    QString lastError() const;
    JobPrep::Data::Database& database() const;


private:
    Database& m_database;
};

}  // namespace JobPrep::Data
