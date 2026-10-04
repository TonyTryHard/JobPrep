#include "data/TrackRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include "data/Database.h"
#include "data/SqlStatement.h"
#include "domain/Structs.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

namespace {

Domain::Track readTrack(const QSqlQuery& query) {
    Domain::Track track;
    track.id = query.value(0).toInt();
    track.name = query.value(1).toString();
    track.color = query.value(2).toString();
    track.icon = query.value(3).toString();
    track.position = query.value(4).toInt();
    return track;
}

bool reportMissingTrack(Database& database, int id) {
    database.setExpectedError(u"No track with id %1."_s.arg(QString::number(id)));
    return false;
}

}  // namespace

TrackRepository::TrackRepository(Database& database, QObject* parent)
    : QObject(parent), m_database(database) {}

TrackRepository::~TrackRepository() = default;

QList<Domain::Track> TrackRepository::all() const {
    QList<Domain::Track> tracks;
    QSqlQuery query(m_database.connection());
    if (!query.exec(u"SELECT id, name, color, icon, position FROM tracks "
                    u"ORDER BY position, id"_s)) {
        m_database.setError(query.lastError().text());
        return tracks;
    }
    while (query.next()) tracks.append(readTrack(query));
    return tracks;
}

std::optional<Domain::Track> TrackRepository::byId(int id) const {
    SqlStatement statement(m_database,
                           u"SELECT id, name, color, icon, position FROM tracks WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readTrack(statement.query());
}

std::optional<int> TrackRepository::idByName(QStringView name) const {
    SqlStatement statement(m_database, u"SELECT id FROM tracks WHERE name = ?"_s);
    statement.bind(1, name.toString());
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return statement.query().value(0).toInt();
}

bool TrackRepository::insert(Domain::Track& track) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(
        m_database, u"INSERT INTO tracks (name, color, icon, position) VALUES (?, ?, ?, ?)"_s);
    statement.bind(1, track.name);
    statement.bind(2, track.color);
    statement.bind(3, track.icon);
    statement.bind(4, track.position);
    if (!statement.exec() || !transaction.commit()) return false;

    track.id = statement.lastInsertId();
    emit changed();
    return true;
}

bool TrackRepository::update(const Domain::Track& track) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"UPDATE tracks SET name = ?, color = ?, icon = ?, "
                                      u"position = ? WHERE id = ?"_s);
    statement.bind(1, track.name);
    statement.bind(2, track.color);
    statement.bind(3, track.icon);
    statement.bind(4, track.position);
    statement.bind(5, track.id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingTrack(m_database, track.id);
    bool owner = transaction.isOwner();
    if (!transaction.commit()) return false;
    if (owner) emit changed();
    else m_database.deferChanged(this);
    return true;
}

bool TrackRepository::remove(int id) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM tracks WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingTrack(m_database, id);
    bool owner = transaction.isOwner();
    if (!transaction.commit()) return false;
    if (owner) emit changed();
    else m_database.deferChanged(this);
    return true;
}

int TrackRepository::count() const {
    SqlStatement statement(m_database, u"SELECT COUNT(*) FROM tracks"_s);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}

bool TrackRepository::isEmpty() const {
    return count() == 0;
}

QString TrackRepository::lastError() const {
    return m_database.lastError();
}

Database& TrackRepository::database() const {
    return m_database;
}

}  // namespace JobPrep::Data
