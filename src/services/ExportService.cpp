#include "services/ExportService.h"
#include <QFile>
#include <QSaveFile>

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

QString escapeField(const QString& field) {
    if (field.isEmpty()) return field;
    const bool needsQuotes = field.contains(u","_s) || field.contains(u"\""_s) ||
                             field.contains(u"\n"_s) || field.contains(u"\r"_s);
    if (!needsQuotes) return field;
    QString escaped = field;
    escaped.replace(u"\""_s, u"\"\""_s);
    return u"\""_s + escaped + u"\""_s;
}

}  // namespace

QStringList ExportRow::toFields() const {
    // Order pinned by ExportService::headers(); the test compares the two.
    return {company,        position,      statusLabel,  applied,      nextStepText,
            salaryText,     source,        updated,      url,          resumeVersion,
            location,       workModeLabel, salaryMin,    salaryMax,    currency,
            nextAction,     nextActionDate, contactName, contactEmail, notes,
            createdAt,      updatedAt};
}

QStringList ExportService::headers() {
    // SPEC §3.3 A1 columns first, then every remaining field of `applications`.
    return {u"Company"_s,         u"Position"_s,       u"Status"_s,      u"Applied"_s,
            u"Next step"_s,       u"Salary"_s,         u"Source"_s,      u"Updated"_s,
            u"URL"_s,             u"Resume version"_s, u"Location"_s,   u"Work mode"_s,
            u"Salary min"_s,      u"Salary max"_s,     u"Currency"_s,   u"Next action"_s,
            u"Next action date"_s, u"Contact name"_s,  u"Contact email"_s, u"Notes"_s,
            u"Created at"_s,      u"Updated at"_s};
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

    // Excel needs the BOM to read UTF-8; §6 requires it.
    const QByteArray bom = QByteArray("\xEF\xBB\xBF", 3);
    file.write(bom);

    const QStringList headerFields = headers.isEmpty() ? ExportService::headers() : headers;
    file.write(headerFields.join(u","_s).toUtf8());
    // CRLF explicitly: Qt's Text flag would emit LF on Linux, which Excel mishandles.
    file.write(u"\r\n"_s.toUtf8());

    for (const auto& row : rows) {
        QStringList fields;
        fields.reserve(row.toFields().size());
        for (const QString& field : row.toFields()) fields.append(escapeField(field));
        file.write(fields.join(u","_s).toUtf8());
        file.write(u"\r\n"_s.toUtf8());
    }

    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

}  // namespace JobPrep::Services