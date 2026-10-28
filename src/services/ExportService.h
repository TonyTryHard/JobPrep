#pragma once

#include <functional>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include "domain/Structs.h"

namespace JobPrep::Data {
class Database;
}

namespace JobPrep::Services {

/// Error kinds for export operations (SPEC §10, Amendment C3).
enum class ExportErrorKind {
    None,
    CannotOpenFile,
    WriteFailed,
    CommitFailed,
    DatabaseError
};

/// Result of an export operation with structured error kind and context.
struct ExportResult {
    bool success{false};
    ExportErrorKind errorKind{ExportErrorKind::None};
    QString errorDetail;
};

/// One row of the applications CSV. The field order is fixed by `ExportService::headers()`
/// and is written by `toFields()`; the two must stay in step.
struct ExportRow {
    QString company;
    QString position;
    QString statusLabel;
    QString applied;           // yyyy-MM-dd or empty
    QString nextStepText;
    QString salaryText;
    QString source;
    QString updated;           // yyyy-MM-dd or empty
    QString url;
    QString resumeVersion;
    QString location;
    QString workModeLabel;
    QString salaryMin;
    QString salaryMax;
    QString currency;
    QString nextAction;
    QString nextActionDate;    // yyyy-MM-dd or empty
    QString contactName;
    QString contactEmail;
    QString notes;
    QString createdAt;         // yyyy-MM-dd or empty
    QString updatedAt;         // yyyy-MM-dd or empty

    /// The fields in CSV order, matching `ExportService::headers()` exactly.
    QStringList toFields() const;
};

class ExportService {
public:
    ExportService() = default;

    // --- CSV Export (SPEC §3.3 A1, §10) ---
    static QStringList headers();

    /// Writes `rows` with CRLF line endings and a UTF-8 BOM. An empty `headers` falls
    /// back to `headers()`.
    bool exportToFile(const QList<ExportRow>& rows,
                      const QStringList& headers,
                      const QString& filePath,
                      QString* error = nullptr) const;

    /// Exports applications from database to a CSV file atomically with UTF-8 BOM.
    ExportResult exportApplicationsCsv(Data::Database& db,
                                       const QString& filePath) const;

    /// Exports in-memory application list to a CSV file atomically with UTF-8 BOM.
    ExportResult exportApplicationsCsv(const QList<Domain::JobApplication>& apps,
                                       const QString& filePath) const;

    // --- ICS Export (SPEC §10, Amendment C5) ---
    /// UI-independent mapping from domain enum to display string for ICS summary.
    static QString interviewTypeLabel(Domain::InterviewType type);
    /// Escapes text for RFC 5545 (\, ;, ,, \n).
    static QString escapeIcsText(const QString& text);
    /// Folds a line at 75 octets without splitting UTF-8 sequences, appending \r\n.
    static QByteArray foldIcsLine(const QString& line);

    /// Generates pure ICS RFC 5545 bytes with CRLF line endings.
    static QByteArray generateIcs(const QList<Domain::Interview>& interviews,
                                  const QHash<int, Domain::JobApplication>& applications,
                                  std::function<QDateTime()> clock = {});

    /// Exports interviews from database to an .ics file atomically using QSaveFile.
    ExportResult exportInterviewsToIcs(Data::Database& db,
                                       const QString& filePath,
                                       std::function<QDateTime()> clock = {}) const;

    /// Exports in-memory interview list to an .ics file atomically.
    ExportResult exportInterviewsToIcs(const QList<Domain::Interview>& interviews,
                                       const QHash<int, Domain::JobApplication>& applications,
                                       const QString& filePath,
                                       std::function<QDateTime()> clock = {}) const;

    // --- JSON Backup (SPEC §10, Amendment C4) ---
    /// Generates backup JSON payload from all database tables within one read transaction.
    static QByteArray generateBackupJson(Data::Database& db,
                                         std::function<QDateTime()> clock = {});

    /// Exports full database backup to a .json file atomically using QSaveFile.
    ExportResult exportBackupJson(Data::Database& db,
                                  const QString& filePath,
                                  std::function<QDateTime()> clock = {}) const;
};

}  // namespace JobPrep::Services