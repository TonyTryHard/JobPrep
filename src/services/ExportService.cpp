#include "services/ExportService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include "data/Database.h"
#include "data/DbFormat.h"
#include "domain/EnumStrings.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

QString escapeCsvField(const QString& field) {
    if (field.isEmpty()) return field;
    const bool needsQuotes = field.contains(u","_s) || field.contains(u"\""_s) ||
                             field.contains(u"\n"_s) || field.contains(u"\r"_s);
    if (!needsQuotes) return field;
    QString escaped = field;
    escaped.replace(u"\""_s, u"\"\""_s);
    return u"\""_s + escaped + u"\""_s;
}

ExportResult writeAtomicFile(const QString& filePath, const QByteArray& data) {
    QSaveFile file(filePath);
    // Opened WITHOUT QIODevice::Text so CRLF line endings are written explicitly byte-for-byte.
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {false, ExportErrorKind::CannotOpenFile, file.errorString()};
    }
    if (file.write(data) != data.size()) {
        return {false, ExportErrorKind::WriteFailed, file.errorString()};
    }
    if (!file.commit()) {
        return {false, ExportErrorKind::CommitFailed, file.errorString()};
    }
    return {true, ExportErrorKind::None, QString()};
}

}  // namespace

QStringList ExportRow::toFields() const {
    return {company,        position,       statusLabel,  applied,      nextStepText,
            salaryText,     source,         updated,      url,          resumeVersion,
            location,       workModeLabel,  salaryMin,    salaryMax,    currency,
            nextAction,     nextActionDate, contactName,  contactEmail, notes,
            createdAt,      updatedAt};
}

QStringList ExportService::headers() {
    return {u"Company"_s,          u"Position"_s,      u"Status"_s,       u"Applied"_s,
            u"Next step"_s,        u"Salary"_s,        u"Source"_s,       u"Updated"_s,
            u"URL"_s,              u"Resume version"_s, u"Location"_s,    u"Work mode"_s,
            u"Salary min"_s,       u"Salary max"_s,    u"Currency"_s,     u"Next action"_s,
            u"Next action date"_s, u"Contact name"_s,  u"Contact email"_s, u"Notes"_s,
            u"Created at"_s,       u"Updated at"_s};
}

bool ExportService::exportToFile(const QList<ExportRow>& rows,
                                 const QStringList& headers,
                                 const QString& filePath,
                                 QString* error) const {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = file.errorString();
        return false;
    }

    const QByteArray bom = QByteArray("\xEF\xBB\xBF", 3);
    file.write(bom);

    const QStringList headerFields = headers.isEmpty() ? ExportService::headers() : headers;
    file.write(headerFields.join(u","_s).toUtf8());
    file.write(u"\r\n"_s.toUtf8());

    for (const auto& row : rows) {
        QStringList fields;
        fields.reserve(row.toFields().size());
        for (const QString& field : row.toFields()) fields.append(escapeCsvField(field));
        file.write(fields.join(u","_s).toUtf8());
        file.write(u"\r\n"_s.toUtf8());
    }

    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

ExportResult ExportService::exportApplicationsCsv(const QList<Domain::JobApplication>& apps,
                                                 const QString& filePath) const {
    QList<ExportRow> rows;
    rows.reserve(apps.size());
    for (const auto& app : apps) {
        ExportRow r;
        r.company = app.company;
        r.position = app.position;
        r.statusLabel = Domain::EnumStrings::toString(app.status);
        r.applied = app.appliedDate.has_value() ? app.appliedDate->toString(Qt::ISODate) : QString();
        r.nextStepText = app.nextAction;
        r.salaryText = app.salaryMin.has_value() && app.salaryMax.has_value()
                           ? u"%1 - %2 %3"_s.arg(*app.salaryMin).arg(*app.salaryMax).arg(app.currency)
                           : (app.salaryMin.has_value() ? u"%1 %2"_s.arg(*app.salaryMin).arg(app.currency) : QString());
        r.source = app.source;
        r.updated = app.updatedAt.toString(Qt::ISODate);
        r.url = app.url;
        r.resumeVersion = app.resumeVersion;
        r.location = app.location;
        r.workModeLabel = Domain::EnumStrings::toString(app.workMode);
        r.salaryMin = app.salaryMin.has_value() ? QString::number(*app.salaryMin) : QString();
        r.salaryMax = app.salaryMax.has_value() ? QString::number(*app.salaryMax) : QString();
        r.currency = app.currency;
        r.nextAction = app.nextAction;
        r.nextActionDate = app.nextActionDate.has_value() ? app.nextActionDate->toString(Qt::ISODate) : QString();
        r.contactName = app.contactName;
        r.contactEmail = app.contactEmail;
        r.notes = app.notes;
        r.createdAt = app.createdAt.toString(Qt::ISODate);
        r.updatedAt = app.updatedAt.toString(Qt::ISODate);
        rows.append(r);
    }

    QString error;
    if (!exportToFile(rows, headers(), filePath, &error)) {
        return {false, ExportErrorKind::CannotOpenFile, error};
    }
    return {true, ExportErrorKind::None, {}};
}

ExportResult ExportService::exportApplicationsCsv(Data::Database& db,
                                                 const QString& filePath) const {
    QSqlQuery q(db.connection());
    if (!q.exec(u"SELECT id, company, position, url, source, resume_version, location, work_mode, salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, contact_name, contact_email, notes, created_at, updated_at FROM applications ORDER BY id ASC;"_s)) {
        return {false, ExportErrorKind::DatabaseError, q.lastError().text()};
    }

    QList<Domain::JobApplication> apps;
    while (q.next()) {
        Domain::JobApplication a;
        a.id = q.value(0).toInt();
        a.company = q.value(1).toString();
        a.position = q.value(2).toString();
        a.url = q.value(3).toString();
        a.source = q.value(4).toString();
        a.resumeVersion = q.value(5).toString();
        a.location = q.value(6).toString();
        a.workMode = Domain::EnumStrings::workModeFromString(q.value(7).toString()).value_or(Domain::WorkMode::Remote);
        if (!q.value(8).isNull()) a.salaryMin = q.value(8).toInt();
        if (!q.value(9).isNull()) a.salaryMax = q.value(9).toInt();
        a.currency = q.value(10).toString();
        a.status = Domain::EnumStrings::applicationStatusFromString(q.value(11).toString()).value_or(Domain::ApplicationStatus::Applied);
        if (!q.value(12).isNull() && !q.value(12).toString().isEmpty()) a.appliedDate = QDate::fromString(q.value(12).toString(), Qt::ISODate);
        a.nextAction = q.value(13).toString();
        if (!q.value(14).isNull() && !q.value(14).toString().isEmpty()) a.nextActionDate = QDate::fromString(q.value(14).toString(), Qt::ISODate);
        a.contactName = q.value(15).toString();
        a.contactEmail = q.value(16).toString();
        a.notes = q.value(17).toString();
        a.createdAt = QDateTime::fromString(q.value(18).toString(), Qt::ISODate);
        a.updatedAt = QDateTime::fromString(q.value(19).toString(), Qt::ISODate);
        apps.append(a);
    }
    return exportApplicationsCsv(apps, filePath);
}

QString ExportService::interviewTypeLabel(Domain::InterviewType type) {
    switch (type) {
        case Domain::InterviewType::Hr:
            return u"HR Screen"_s;
        case Domain::InterviewType::Technical:
            return u"Technical"_s;
        case Domain::InterviewType::LiveCoding:
            return u"Live Coding"_s;
        case Domain::InterviewType::SystemDesign:
            return u"System Design"_s;
        case Domain::InterviewType::Behavioral:
            return u"Behavioral"_s;
        case Domain::InterviewType::Final:
            return u"Final"_s;
        case Domain::InterviewType::Other:
            return u"Other"_s;
    }
    return u"Other"_s;
}

QString ExportService::escapeIcsText(const QString& text) {
    QString out;
    out.reserve(text.size() + 16);
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text[i];
        if (c == u'\\') {
            out += u"\\\\"_s;
        } else if (c == u';') {
            out += u"\\;"_s;
        } else if (c == u',') {
            out += u"\\,"_s;
        } else if (c == u'\r') {
            if (i + 1 < text.size() && text[i + 1] == u'\n') {
                out += u"\\n"_s;
                ++i;
            } else {
                out += u"\\n"_s;
            }
        } else if (c == u'\n') {
            out += u"\\n"_s;
        } else {
            out += c;
        }
    }
    return out;
}

QByteArray ExportService::foldIcsLine(const QString& line) {
    const QByteArray full = line.toUtf8();
    if (full.size() <= 75) {
        return full + "\r\n";
    }

    QByteArray result;
    qsizetype lineOctets = 0;
    qsizetype i = 0;
    while (i < line.size()) {
        QString chStr;
        if (line[i].isHighSurrogate() && i + 1 < line.size() && line[i + 1].isLowSurrogate()) {
            chStr = line.mid(i, 2);
            i += 2;
        } else {
            chStr = line.mid(i, 1);
            i += 1;
        }
        const QByteArray chBytes = chStr.toUtf8();

        if (lineOctets + chBytes.size() > 75) {
            result += "\r\n ";
            result += chBytes;
            lineOctets = 1 + chBytes.size();
        } else {
            result += chBytes;
            lineOctets += chBytes.size();
        }
    }
    result += "\r\n";
    return result;
}

QByteArray ExportService::generateIcs(const QList<Domain::Interview>& interviews,
                                      const QHash<int, Domain::JobApplication>& applications,
                                      std::function<QDateTime()> clock) {
    const QDateTime now = clock ? clock() : QDateTime::currentDateTime();
    const QString dtstamp = now.toUTC().toString(u"yyyyMMddTHHmmss'Z'"_s);

    QByteArray out;
    out += foldIcsLine(u"BEGIN:VCALENDAR"_s);
    out += foldIcsLine(u"VERSION:2.0"_s);
    out += foldIcsLine(u"PRODID:-//JobPrep//JobPrep Desktop//EN"_s);
    out += foldIcsLine(u"CALSCALE:GREGORIAN"_s);

    for (const auto& interview : interviews) {
        // Cancelled interviews are not exported (SPEC §10, Amendment C5, §14)
        if (interview.outcome == Domain::InterviewOutcome::Cancelled) {
            continue;
        }

        QString company = u"Unknown Company"_s;
        QString position;
        const auto it = applications.constFind(interview.applicationId);
        if (it != applications.constEnd()) {
            company = it->company;
            position = it->position;
        }

        const QDateTime start = interview.startAt;
        if (!start.isValid()) {
            continue;
        }
        const QDateTime end = start.addSecs(interview.durationMin * 60);

        out += foldIcsLine(u"BEGIN:VEVENT"_s);
        out += foldIcsLine(u"UID:jobprep-interview-%1@jobprep.local"_s.arg(interview.id));
        out += foldIcsLine(u"DTSTAMP:%1"_s.arg(dtstamp));
        out += foldIcsLine(u"DTSTART:%1"_s.arg(start.toString(u"yyyyMMddTHHmmss"_s)));
        out += foldIcsLine(u"DTEND:%1"_s.arg(end.toString(u"yyyyMMddTHHmmss"_s)));

        const QString summaryText = u"Interview: %1 — %2"_s.arg(company, interviewTypeLabel(interview.type));
        out += foldIcsLine(u"SUMMARY:%1"_s.arg(escapeIcsText(summaryText)));

        if (!interview.place.trimmed().isEmpty()) {
            out += foldIcsLine(u"LOCATION:%1"_s.arg(escapeIcsText(interview.place.trimmed())));
        }

        QStringList descLines;
        if (!position.trimmed().isEmpty()) {
            descLines.append(u"Position: %1"_s.arg(position.trimmed()));
        }
        if (!interview.interviewer.trimmed().isEmpty()) {
            descLines.append(u"Interviewer: %1"_s.arg(interview.interviewer.trimmed()));
        }
        if (!interview.notes.trimmed().isEmpty()) {
            descLines.append(u"Notes: %1"_s.arg(interview.notes.trimmed()));
        }
        if (!descLines.isEmpty()) {
            out += foldIcsLine(u"DESCRIPTION:%1"_s.arg(escapeIcsText(descLines.join(u"\n"_s))));
        }

        // VALARM display 60 minutes before
        out += foldIcsLine(u"BEGIN:VALARM"_s);
        out += foldIcsLine(u"ACTION:DISPLAY"_s);
        out += foldIcsLine(u"DESCRIPTION:%1"_s.arg(escapeIcsText(summaryText)));
        out += foldIcsLine(u"TRIGGER:-PT60M"_s);
        out += foldIcsLine(u"END:VALARM"_s);

        out += foldIcsLine(u"END:VEVENT"_s);
    }

    out += foldIcsLine(u"END:VCALENDAR"_s);
    return out;
}

ExportResult ExportService::exportInterviewsToIcs(const QList<Domain::Interview>& interviews,
                                                 const QHash<int, Domain::JobApplication>& applications,
                                                 const QString& filePath,
                                                 std::function<QDateTime()> clock) const {
    const QByteArray data = generateIcs(interviews, applications, clock);
    return writeAtomicFile(filePath, data);
}

ExportResult ExportService::exportInterviewsToIcs(Data::Database& db,
                                                 const QString& filePath,
                                                 std::function<QDateTime()> clock) const {
    QSqlQuery query(db.connection());
    if (!query.exec(u"BEGIN TRANSACTION"_s)) {
        return {false, ExportErrorKind::DatabaseError, query.lastError().text()};
    }

    QHash<int, Domain::JobApplication> applications;
    if (query.exec(u"SELECT id, company, position FROM applications"_s)) {
        while (query.next()) {
            Domain::JobApplication app;
            app.id = query.value(0).toInt();
            app.company = query.value(1).toString();
            app.position = query.value(2).toString();
            applications.insert(app.id, app);
        }
    }

    QList<Domain::Interview> interviews;
    if (query.exec(u"SELECT id, application_id, start_at, duration_min, type, place, interviewer, notes, outcome FROM interviews ORDER BY start_at ASC"_s)) {
        while (query.next()) {
            Domain::Interview iv;
            iv.id = query.value(0).toInt();
            iv.applicationId = query.value(1).toInt();
            iv.startAt = Data::DbFormat::dateTimeFromText(query.value(2).toString()).value_or(QDateTime());
            iv.durationMin = query.value(3).toInt();
            iv.type = Domain::EnumStrings::interviewTypeFromString(query.value(4).toString()).value_or(Domain::InterviewType::Other);
            iv.place = query.value(5).toString();
            iv.interviewer = query.value(6).toString();
            iv.notes = query.value(7).toString();
            iv.outcome = Domain::EnumStrings::interviewOutcomeFromString(query.value(8).toString()).value_or(Domain::InterviewOutcome::Pending);
            interviews.append(iv);
        }
    }
    query.exec(u"COMMIT"_s);

    const QByteArray data = generateIcs(interviews, applications, clock);
    return writeAtomicFile(filePath, data);
}

QByteArray ExportService::generateBackupJson(Data::Database& db, std::function<QDateTime()> clock) {
    const QDateTime now = clock ? clock() : QDateTime::currentDateTime();
    const QString exportedAt = now.toUTC().toString(Qt::ISODateWithMs);

    QJsonObject root;
    root[u"format"_s] = u"jobprep-backup"_s;
    root[u"version"_s] = 1;
    root[u"exportedAt"_s] = exportedAt;

    QSqlQuery q(db.connection());
    q.exec(u"BEGIN TRANSACTION"_s);

    // 1. tracks
    QJsonArray tracksArray;
    if (q.exec(u"SELECT id, name, color, icon, position FROM tracks ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"name"_s] = q.value(1).toString();
            obj[u"color"_s] = q.value(2).toString();
            obj[u"icon"_s] = q.value(3).toString();
            obj[u"position"_s] = q.value(4).toInt();
            tracksArray.append(obj);
        }
    }
    root[u"tracks"_s] = tracksArray;

    // 2. topics
    QJsonArray topicsArray;
    if (q.exec(u"SELECT id, track_id, title, status, priority, target_date, notes, resources, tags, position, created_at, updated_at FROM topics ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"track_id"_s] = q.value(1).toInt();
            obj[u"title"_s] = q.value(2).toString();
            obj[u"status"_s] = q.value(3).toString();
            obj[u"priority"_s] = q.value(4).toString();
            obj[u"target_date"_s] = q.value(5).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(5).toString());
            obj[u"notes"_s] = q.value(6).toString();
            obj[u"resources"_s] = q.value(7).toString();
            obj[u"tags"_s] = q.value(8).toString();
            obj[u"position"_s] = q.value(9).toInt();
            obj[u"created_at"_s] = q.value(10).toString();
            obj[u"updated_at"_s] = q.value(11).toString();
            topicsArray.append(obj);
        }
    }
    root[u"topics"_s] = topicsArray;

    // 3. subtasks
    QJsonArray subtasksArray;
    if (q.exec(u"SELECT id, topic_id, text, done, position FROM subtasks ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"topic_id"_s] = q.value(1).toInt();
            obj[u"text"_s] = q.value(2).toString();
            obj[u"done"_s] = q.value(3).toInt();
            obj[u"position"_s] = q.value(4).toInt();
            subtasksArray.append(obj);
        }
    }
    root[u"subtasks"_s] = subtasksArray;

    // 4. sessions (table: study_sessions)
    QJsonArray sessionsArray;
    if (q.exec(u"SELECT id, topic_id, session_date, minutes, note FROM study_sessions ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"topic_id"_s] = q.value(1).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(1).toInt());
            obj[u"session_date"_s] = q.value(2).toString();
            obj[u"minutes"_s] = q.value(3).toInt();
            obj[u"note"_s] = q.value(4).toString();
            sessionsArray.append(obj);
        }
    }
    root[u"sessions"_s] = sessionsArray;

    // 5. applications
    QJsonArray appsArray;
    if (q.exec(u"SELECT id, company, position, url, source, resume_version, location, work_mode, salary_min, salary_max, currency, status, applied_date, next_action, next_action_date, contact_name, contact_email, notes, created_at, updated_at FROM applications ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"company"_s] = q.value(1).toString();
            obj[u"position"_s] = q.value(2).toString();
            obj[u"url"_s] = q.value(3).toString();
            obj[u"source"_s] = q.value(4).toString();
            obj[u"resume_version"_s] = q.value(5).toString();
            obj[u"location"_s] = q.value(6).toString();
            obj[u"work_mode"_s] = q.value(7).toString();
            obj[u"salary_min"_s] = q.value(8).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(8).toInt());
            obj[u"salary_max"_s] = q.value(9).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(9).toInt());
            obj[u"currency"_s] = q.value(10).toString();
            obj[u"status"_s] = q.value(11).toString();
            obj[u"applied_date"_s] = q.value(12).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(12).toString());
            obj[u"next_action"_s] = q.value(13).toString();
            obj[u"next_action_date"_s] = q.value(14).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(14).toString());
            obj[u"contact_name"_s] = q.value(15).toString();
            obj[u"contact_email"_s] = q.value(16).toString();
            obj[u"notes"_s] = q.value(17).toString();
            obj[u"created_at"_s] = q.value(18).toString();
            obj[u"updated_at"_s] = q.value(19).toString();
            appsArray.append(obj);
        }
    }
    root[u"applications"_s] = appsArray;

    // 6. statusHistory (table: status_history)
    QJsonArray historyArray;
    if (q.exec(u"SELECT id, application_id, from_status, to_status, changed_at, note FROM status_history ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"application_id"_s] = q.value(1).toInt();
            obj[u"from_status"_s] = q.value(2).isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(q.value(2).toString());
            obj[u"to_status"_s] = q.value(3).toString();
            obj[u"changed_at"_s] = q.value(4).toString();
            obj[u"note"_s] = q.value(5).toString();
            historyArray.append(obj);
        }
    }
    root[u"statusHistory"_s] = historyArray;

    // 7. interviews
    QJsonArray interviewsArray;
    if (q.exec(u"SELECT id, application_id, start_at, duration_min, type, place, interviewer, notes, outcome FROM interviews ORDER BY id ASC"_s)) {
        while (q.next()) {
            QJsonObject obj;
            obj[u"id"_s] = q.value(0).toInt();
            obj[u"application_id"_s] = q.value(1).toInt();
            obj[u"start_at"_s] = q.value(2).toString();
            obj[u"duration_min"_s] = q.value(3).toInt();
            obj[u"type"_s] = q.value(4).toString();
            obj[u"place"_s] = q.value(5).toString();
            obj[u"interviewer"_s] = q.value(6).toString();
            obj[u"notes"_s] = q.value(7).toString();
            obj[u"outcome"_s] = q.value(8).toString();
            interviewsArray.append(obj);
        }
    }
    root[u"interviews"_s] = interviewsArray;

    q.exec(u"COMMIT"_s);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

ExportResult ExportService::exportBackupJson(Data::Database& db,
                                             const QString& filePath,
                                             std::function<QDateTime()> clock) const {
    const QByteArray json = generateBackupJson(db, clock);
    return writeAtomicFile(filePath, json);
}

}  // namespace JobPrep::Services
