#include "data/TopicRepository.h"

#include <QMap>
#include <QSet>
#include <QSqlQuery>
#include <QVariant>
#include "data/Database.h"
#include "data/DbFormat.h"
#include "data/SqlStatement.h"
#include "domain/EnumStrings.h"
#include "domain/Structs.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

namespace {

/// Column list shared by every topic query (SPEC §5).
QString selectTopics() {
    static const QString sql =
        u"SELECT id, track_id, title, status, priority, target_date, notes, resources, tags, "
        u"position, created_at, updated_at FROM topics"_s;
    return sql;
}

QString selectSubtasks() {
    static const QString sql =
        u"SELECT id, topic_id, text, done, position FROM subtasks"_s;
    return sql;
}

const QString& orderBySubtaskPosition() {
    static const QString sql = u" ORDER BY position, id"_s;
    return sql;
}

std::optional<Domain::Topic> readTopic(Database& database, const QSqlQuery& query) {
    const auto status = Domain::EnumStrings::topicStatusFromString(query.value(3).toString());
    const auto priority = Domain::EnumStrings::priorityFromString(query.value(4).toString());
    if (!status || !priority) {
        database.setError(u"Unknown topic status or priority in row %1."_s.arg(
            QString::number(query.value(0).toInt())));
        return std::nullopt;
    }

    Domain::Topic topic;
    topic.id = query.value(0).toInt();
    topic.trackId = query.value(1).toInt();
    topic.title = query.value(2).toString();
    topic.status = *status;
    topic.priority = *priority;
    topic.targetDate = DbFormat::dateFromValue(query.value(5));
    topic.notes = query.value(6).toString();
    topic.resources = query.value(7).toString();
    topic.tags = query.value(8).toString();
    topic.position = query.value(9).toInt();
    topic.createdAt = DbFormat::dateTimeFromValue(query.value(10)).value_or(QDateTime());
    topic.updatedAt = DbFormat::dateTimeFromValue(query.value(11)).value_or(QDateTime());
    return topic;
}

Domain::Subtask readSubtask(const QSqlQuery& query) {
    Domain::Subtask subtask;
    subtask.id = query.value(0).toInt();
    subtask.topicId = query.value(1).toInt();
    subtask.text = query.value(2).toString();
    subtask.done = query.value(3).toInt() != 0;
    subtask.position = query.value(4).toInt();
    return subtask;
}

bool reportMissingRow(Database& database, const QString& what, int id) {
    database.setExpectedError(u"No %1 with id %2."_s.arg(what, QString::number(id)));
    return false;
}

void bindTopic(SqlStatement& statement, const Domain::Topic& topic) {
    statement.bind(1, topic.trackId);
    statement.bind(2, topic.title);
    statement.bind(3, Domain::EnumStrings::toString(topic.status));
    statement.bind(4, Domain::EnumStrings::toString(topic.priority));
    statement.bind(5, topic.targetDate);
    statement.bind(6, topic.notes);
    statement.bind(7, topic.resources);
    statement.bind(8, topic.tags);
    statement.bind(9, topic.position);
}

}  // namespace

TopicRepository::TopicRepository(Database& database, QObject* parent)
    : Repository(parent), m_database(database) {}

TopicRepository::~TopicRepository() = default;

QList<Domain::Topic> TopicRepository::all() const {
    QList<Domain::Topic> topics;
    SqlStatement statement(m_database, selectTopics() + u" ORDER BY position, id"_s);
    if (!statement.exec()) return topics;
    while (statement.query().next()) {
        if (auto topic = readTopic(m_database, statement.query())) topics.append(*topic);
    }
    return topics;
}

QList<Domain::Topic> TopicRepository::byTrack(int trackId) const {
    QList<Domain::Topic> topics;
    SqlStatement statement(m_database, selectTopics() + u" WHERE track_id = ? "
                                              u"ORDER BY position, id"_s);
    statement.bind(1, trackId);
    if (!statement.exec()) return topics;
    while (statement.query().next()) {
        if (auto topic = readTopic(m_database, statement.query())) topics.append(*topic);
    }
    return topics;
}

QList<Domain::Topic> TopicRepository::byTargetDateRange(QDate from, QDate to) const {
    QList<Domain::Topic> topics;
    SqlStatement statement(m_database,
                           selectTopics() +
                               u" WHERE target_date >= ? AND target_date <= ? "
                               u"AND status != 'done' "
                               u"ORDER BY target_date, position, id"_s);
    statement.bind(1, DbFormat::toText(from));
    statement.bind(2, DbFormat::toText(to));
    if (!statement.exec()) return topics;
    while (statement.query().next()) {
        if (auto topic = readTopic(m_database, statement.query())) topics.append(*topic);
    }
    return topics;
}

std::optional<Domain::Topic> TopicRepository::byId(int id) const {
    SqlStatement statement(m_database, selectTopics() + u" WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec() || !statement.query().next()) return std::nullopt;
    return readTopic(m_database, statement.query());
}

bool TopicRepository::insert(Domain::Topic& topic) {
    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    topic.createdAt = DbFormat::now();
    topic.updatedAt = topic.createdAt;

    SqlStatement statement(m_database,
                           u"INSERT INTO topics (track_id, title, status, priority, target_date, "
                           u"notes, resources, tags, position, created_at, updated_at) "
                           u"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"_s);
    bindTopic(statement, topic);
    statement.bind(10, topic.createdAt);
    statement.bind(11, topic.updatedAt);
    if (!statement.exec()) return false;
    if (!commitAndNotify(m_database, transaction)) return false;
    topic.id = statement.lastInsertId();
    return true;
}

bool TopicRepository::update(const Domain::Topic& topic) {
    if (!acceptExisting(m_database, byId(topic.id).has_value(), u"topic"_s, topic.id)) return false;

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"UPDATE topics SET track_id = ?, title = ?, status = ?, priority = ?, "
                           u"target_date = ?, notes = ?, resources = ?, tags = ?, position = ?, "
                           u"updated_at = ? WHERE id = ?"_s);
    bindTopic(statement, topic);
    statement.bind(10, DbFormat::now());
    statement.bind(11, topic.id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingRow(m_database, u"topic"_s, topic.id);
    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::setStatus(int id, Domain::TopicStatus status) {
    if (!acceptExisting(m_database, byId(id).has_value(), u"topic"_s, id)) return false;

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(
        m_database, u"UPDATE topics SET status = ?, updated_at = ? WHERE id = ?"_s);
    statement.bind(1, Domain::EnumStrings::toString(status));
    statement.bind(2, DbFormat::now());
    statement.bind(3, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingRow(m_database, u"topic"_s, id);
    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::updatePositions(const QList<int>& orderedIds) {
    for (const int id : orderedIds) {
        if (!acceptExisting(m_database, byId(id).has_value(), u"topic"_s, id)) return false;
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"UPDATE topics SET position = ? WHERE id = ?"_s);
    for (int position = 0; position < orderedIds.size(); ++position) {
        statement.bind(1, position);
        statement.bind(2, orderedIds.at(position));
        if (!statement.exec()) return false;
        if (statement.rowsAffected() != 1) {
            return reportMissingRow(m_database, u"topic"_s, orderedIds.at(position));
        }
    }
    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::moveTopic(int id, Domain::TopicStatus targetStatus, const QList<int>& orderedIds) {
    const auto targetTopic = byId(id);
    if (!acceptExisting(m_database, targetTopic.has_value(), u"topic"_s, id)) return false;

    if (!orderedIds.contains(id)) {
        m_database.setExpectedError(
            u"Topic %1 is not in target orderedIds."_s.arg(QString::number(id)));
        return false;
    }

    QSet<int> seen;
    for (const int oid : orderedIds) {
        if (seen.contains(oid)) {
            m_database.setExpectedError(
                u"Duplicate id %1 in orderedIds."_s.arg(QString::number(oid)));
            return false;
        }
        seen.insert(oid);
        if (!acceptExisting(m_database, byId(oid).has_value(), u"topic"_s, oid)) return false;
    }

    SqlStatement targetTopicsQuery(
        m_database, selectTopics() + u" WHERE status = ? ORDER BY position, id"_s);
    targetTopicsQuery.bind(1, Domain::EnumStrings::toString(targetStatus));
    if (!targetTopicsQuery.exec()) return false;

    QList<Domain::Topic> existingTargetTopics;
    while (targetTopicsQuery.query().next()) {
        if (auto t = readTopic(m_database, targetTopicsQuery.query())) {
            existingTargetTopics.append(*t);
        }
    }

    // Check no-op: same status and exact same visible order
    if (targetTopic->status == targetStatus) {
        QList<int> currentVisibleOrder;
        for (const auto& t : existingTargetTopics) {
            if (orderedIds.contains(t.id)) {
                currentVisibleOrder.append(t.id);
            }
        }
        if (currentVisibleOrder == orderedIds) {
            return true;
        }
    }

    // Construct the new order of all topics in targetStatus
    QList<Domain::Topic> baseTopics;
    for (const auto& t : existingTargetTopics) {
        if (t.id != id) {
            baseTopics.append(t);
        }
    }

    QList<int> newTargetStatusIds;
    int visibleIdx = 0;
    for (const auto& t : baseTopics) {
        if (orderedIds.contains(t.id)) {
            if (visibleIdx < orderedIds.size()) {
                newTargetStatusIds.append(orderedIds.at(visibleIdx++));
            }
        } else {
            newTargetStatusIds.append(t.id);
        }
    }
    while (visibleIdx < orderedIds.size()) {
        newTargetStatusIds.append(orderedIds.at(visibleIdx++));
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    if (targetTopic->status != targetStatus) {
        SqlStatement updateStatusStmt(
            m_database, u"UPDATE topics SET status = ?, updated_at = ? WHERE id = ?"_s);
        updateStatusStmt.bind(1, Domain::EnumStrings::toString(targetStatus));
        updateStatusStmt.bind(2, DbFormat::now());
        updateStatusStmt.bind(3, id);
        if (!updateStatusStmt.exec()) return false;
        if (updateStatusStmt.rowsAffected() != 1) {
            return reportMissingRow(m_database, u"topic"_s, id);
        }
    }

    SqlStatement allQuery(m_database, selectTopics() + u" ORDER BY position, id"_s);
    if (!allQuery.exec()) return false;

    QMap<Domain::TopicStatus, QList<int>> statusBuckets;
    while (allQuery.query().next()) {
        const int tid = allQuery.query().value(0).toInt();
        const auto optStatus =
            Domain::EnumStrings::topicStatusFromString(allQuery.query().value(3).toString());
        if (optStatus) {
            if (tid != id) {
                statusBuckets[*optStatus].append(tid);
            }
        }
    }

    statusBuckets[targetStatus] = newTargetStatusIds;

    const QList<Domain::TopicStatus> statusOrder = {
        Domain::TopicStatus::Backlog,
        Domain::TopicStatus::InProgress,
        Domain::TopicStatus::Review,
        Domain::TopicStatus::Done,
    };

    QList<int> globalOrderedIds;
    for (const auto st : statusOrder) {
        globalOrderedIds.append(statusBuckets.value(st));
    }

    SqlStatement posStmt(m_database, u"UPDATE topics SET position = ? WHERE id = ?"_s);
    for (int pos = 0; pos < globalOrderedIds.size(); ++pos) {
        posStmt.bind(1, pos);
        posStmt.bind(2, globalOrderedIds.at(pos));
        if (!posStmt.exec()) return false;
        if (posStmt.rowsAffected() != 1) {
            return reportMissingRow(m_database, u"topic"_s, globalOrderedIds.at(pos));
        }
    }

    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::remove(int id) {
    if (!acceptExisting(m_database, byId(id).has_value(), u"topic"_s, id)) return false;

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM topics WHERE id = ?"_s);
    statement.bind(1, id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingRow(m_database, u"topic"_s, id);
    return commitAndNotify(m_database, transaction);
}

int TopicRepository::count() const {
    SqlStatement statement(m_database, u"SELECT COUNT(*) FROM topics"_s);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}

bool TopicRepository::isEmpty() const {
    return count() == 0;
}

QList<Domain::Subtask> TopicRepository::subtasks(int topicId) const {
    QList<Domain::Subtask> subtasks;
    SqlStatement statement(m_database,
                           selectSubtasks() + u" WHERE topic_id = ?"_s + orderBySubtaskPosition());
    statement.bind(1, topicId);
    if (!statement.exec()) return subtasks;
    while (statement.query().next()) subtasks.append(readSubtask(statement.query()));
    return subtasks;
}

bool TopicRepository::addSubtask(Domain::Subtask& subtask) {
    if (!acceptExisting(m_database, byId(subtask.topicId).has_value(), u"topic"_s,
                        subtask.topicId)) {
        return false;
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database,
                           u"INSERT INTO subtasks (topic_id, text, done, position) "
                           u"VALUES (?, ?, ?, ?)"_s);
    statement.bind(1, subtask.topicId);
    statement.bind(2, subtask.text);
    statement.bind(3, subtask.done);
    statement.bind(4, subtask.position);
    if (!statement.exec()) return false;
    if (!commitAndNotify(m_database, transaction)) return false;
    subtask.id = statement.lastInsertId();
    return true;
}

bool TopicRepository::updateSubtask(const Domain::Subtask& subtask) {
    if (!acceptExisting(m_database, subtaskExists(subtask.id), u"subtask"_s, subtask.id)) {
        return false;
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(
        m_database, u"UPDATE subtasks SET topic_id = ?, text = ?, done = ?, position = ? "
                    u"WHERE id = ?"_s);
    statement.bind(1, subtask.topicId);
    statement.bind(2, subtask.text);
    statement.bind(3, subtask.done);
    statement.bind(4, subtask.position);
    statement.bind(5, subtask.id);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingRow(m_database, u"subtask"_s, subtask.id);
    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::setSubtaskDone(int subtaskId, bool done) {
    if (!acceptExisting(m_database, subtaskExists(subtaskId), u"subtask"_s, subtaskId)) {
        return false;
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"UPDATE subtasks SET done = ? WHERE id = ?"_s);
    statement.bind(1, done);
    statement.bind(2, subtaskId);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) {
        return reportMissingRow(m_database, u"subtask"_s, subtaskId);
    }
    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::removeSubtask(int subtaskId) {
    if (!acceptExisting(m_database, subtaskExists(subtaskId), u"subtask"_s, subtaskId)) {
        return false;
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(m_database, u"DELETE FROM subtasks WHERE id = ?"_s);
    statement.bind(1, subtaskId);
    if (!statement.exec()) return false;
    if (statement.rowsAffected() != 1) return reportMissingRow(m_database, u"subtask"_s, subtaskId);
    return commitAndNotify(m_database, transaction);
}

bool TopicRepository::reorderSubtasks(int topicId, const QList<int>& orderedIds) {
    for (const int subtaskId : orderedIds) {
        if (!acceptExisting(m_database, subtaskExists(subtaskId), u"subtask"_s, subtaskId)) {
            return false;
        }
    }

    auto transaction = m_database.transaction();
    if (!transaction.isActive()) return false;

    SqlStatement statement(
        m_database, u"UPDATE subtasks SET position = ? WHERE id = ? AND topic_id = ?"_s);
    for (int position = 0; position < orderedIds.size(); ++position) {
        statement.bind(1, position);
        statement.bind(2, orderedIds.at(position));
        statement.bind(3, topicId);
        if (!statement.exec()) return false;
        if (statement.rowsAffected() != 1) {
            return reportMissingRow(m_database, u"subtask"_s, orderedIds.at(position));
        }
    }
    return commitAndNotify(m_database, transaction);
}

QHash<int, TopicSubtaskProgress> TopicRepository::subtaskProgress() const {
    QHash<int, TopicSubtaskProgress> result;
    SqlStatement statement(
        m_database,
        u"SELECT topic_id, COUNT(*), COALESCE(SUM(done), 0) FROM subtasks GROUP BY topic_id"_s);
    if (!statement.exec()) return result;
    while (statement.query().next()) {
        const int topicId = statement.query().value(0).toInt();
        const int total = statement.query().value(1).toInt();
        const int done = statement.query().value(2).toInt();
        result.insert(topicId, TopicSubtaskProgress{done, total});
    }
    return result;
}

int TopicRepository::subtaskCount(int topicId) const {
    return countSubtasks(topicId, false);
}

int TopicRepository::doneSubtaskCount(int topicId) const {
    return countSubtasks(topicId, true);
}

int TopicRepository::countSubtasks(int topicId, bool doneOnly) const {
    SqlStatement statement(m_database,
                           doneOnly ? u"SELECT COUNT(*) FROM subtasks WHERE topic_id = ? AND "
                                     u"done = 1"_s
                                    : u"SELECT COUNT(*) FROM subtasks WHERE topic_id = ?"_s);
    statement.bind(1, topicId);
    if (!statement.exec() || !statement.query().next()) return 0;
    return statement.query().value(0).toInt();
}

bool TopicRepository::subtaskExists(int subtaskId) const {
    SqlStatement statement(m_database, u"SELECT 1 FROM subtasks WHERE id = ?"_s);
    statement.bind(1, subtaskId);
    return statement.exec() && statement.query().next();
}

QString TopicRepository::lastError() const {
    return m_database.lastError();
}

Database& TopicRepository::database() const {
    return m_database;
}

}  // namespace JobPrep::Data
