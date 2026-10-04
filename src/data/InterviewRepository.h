#pragma once

#include <optional>
#include <QList>
#include <QObject>
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// CRUD for interviews (SPEC §5 `interviews`). Rows belong to one application.
class InterviewRepository : public QObject {
    Q_OBJECT

public:
    explicit InterviewRepository(Database& database, QObject* parent = nullptr);
    ~InterviewRepository() override;

    QList<Domain::Interview> all() const;
    QList<Domain::Interview> byApplication(int applicationId) const;

    /// Interviews starting within [from, to], inclusive.
    QList<Domain::Interview> inRange(const QDateTime& from, const QDateTime& to) const;

    std::optional<Domain::Interview> byId(int id) const;
    std::optional<Domain::Interview> nextUpcoming(const QDateTime& now) const;

    /// Inserts `interview` and fills its id.
    bool insert(Domain::Interview& interview);
    bool update(const Domain::Interview& interview);
    bool remove(int id);

    QString lastError() const;
    JobPrep::Data::Database& database() const;

signals:
    void changed();

private:
    Database& m_database;
};

}  // namespace JobPrep::Data
