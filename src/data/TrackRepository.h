#pragma once

#include <optional>
#include <QList>
#include <QStringView>
#include "data/Repository.h"
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// CRUD for study tracks (SPEC §5 `tracks`). Deleting a track cascades its topics.
class TrackRepository : public Repository {

public:
    explicit TrackRepository(Database& database, QObject* parent = nullptr);
    ~TrackRepository() override = default;

    QList<Domain::Track> all() const;
    std::optional<Domain::Track> byId(int id) const;
    std::optional<int> idByName(QStringView name) const;

    /// Inserts `track` and fills its id.
    bool insert(Domain::Track& track);
    bool update(const Domain::Track& track);
    bool remove(int id);

    int count() const;
    bool isEmpty() const;
    QString lastError() const;
    JobPrep::Data::Database& database() const;


private:
    Database& m_database;
};

}  // namespace JobPrep::Data
