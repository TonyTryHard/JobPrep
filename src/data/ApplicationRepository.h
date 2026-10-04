#pragma once

#include <optional>
#include <QHash>
#include <QList>
#include <QObject>
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// CRUD for job applications and their status history (SPEC §5 `applications`,
/// `status_history`). Status changes write the history row in the same transaction
/// (SPEC §3.3 A5), including the initial row of a new application.
class ApplicationRepository : public QObject {
    Q_OBJECT

public:
    explicit ApplicationRepository(Database& database, QObject* parent = nullptr);
    ~ApplicationRepository() override;

    QList<Domain::JobApplication> all() const;
    std::optional<Domain::JobApplication> byId(int id) const;

    /// Applications with one of `statuses`; an empty list yields an empty result.
    QList<Domain::JobApplication> byStatuses(const QList<Domain::ApplicationStatus>& statuses) const;

    /// Inserts `application`, fills its id, timestamps and the initial history row.
    bool insert(Domain::JobApplication& application);
    bool update(const Domain::JobApplication& application);

    /// Moves an application to `status`, adding one history row when it differs.
    bool setStatus(int id, Domain::ApplicationStatus status, const QString& note = QString());

    bool setNextAction(int id, const QString& text, std::optional<QDate> date);

    /// Copies `id` into `copy` (new id, fresh timestamps, own initial history row).
    bool duplicate(int id, Domain::JobApplication& copy);

    /// Cascades status history and interviews.
    bool remove(int id);

    int count() const;
    bool isEmpty() const;
    QHash<Domain::ApplicationStatus, int> countsByStatus() const;

    QList<Domain::StatusChange> statusHistory(int applicationId) const;
    QList<Domain::StatusChange> allStatusHistory() const;

    QString lastError() const;
    JobPrep::Data::Database& database() const;

signals:
    void changed();

private:
    bool insertRow(Domain::JobApplication& application);
    bool updateRow(const Domain::JobApplication& application);
    bool updateStatusRow(int id, Domain::ApplicationStatus status,
                         const std::optional<QDate>& appliedDate);
    bool addHistoryRow(int applicationId, std::optional<Domain::ApplicationStatus> from,
                       Domain::ApplicationStatus to, const QDateTime& changedAt,
                       const QString& note);

    Database& m_database;
};

}  // namespace JobPrep::Data
