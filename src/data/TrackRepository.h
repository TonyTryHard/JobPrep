#pragma once

#include <optional>
#include <QList>
#include <QObject>
#include <QStringView>
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// CRUD for study tracks (SPEC §5 `tracks`). Deleting a track cascades its topics.
class TrackRepository : public QObject {
    Q_OBJECT

public:
    explicit TrackRepository(Database& database, QObject* parent = nullptr);
    ~TrackRepository() override;

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

signals:
    void changed();

private:
    Database& m_database;
};

}  // namespace JobPrep::Data
