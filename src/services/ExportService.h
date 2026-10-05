#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace JobPrep::Services {

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

    /// The canonical header row of the applications CSV (SPEC §6: the A1 columns first,
    /// then every remaining field). Fixed English, not translated, so an exported file
    /// stays readable in any spreadsheet.
    static QStringList headers();

    /// Writes `rows` with CRLF line endings and a UTF-8 BOM. An empty `headers` falls
    /// back to `headers()`.
    bool exportToFile(const QList<ExportRow>& rows,
                      const QStringList& headers,
                      const QString& filePath,
                      QString* error = nullptr) const;
};

}  // namespace JobPrep::Services