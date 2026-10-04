#include "services/ExportService.h"
#include <QFile>
#include <QSaveFile>
#include <QStringList>

using namespace Qt::StringLiterals;

namespace JobPrep::Services {

namespace {

QString escapeField(const QString& field) {
    if (field.isEmpty()) return field;
    bool needsQuotes = field.contains(u","_s) || field.contains(QStringLiteral("\"")) || field.contains(u"\n"_s) || field.contains(u"\r"_s);
    if (!needsQuotes) return field;
    QString escaped = field;
    escaped.replace(QStringLiteral("\""), u"\"\""_s);
    return u"\""_s + escaped + u"\""_s;
}

}  // namespace

bool ExportService::exportToFile(const QList<ExportRow>& rows,
                                const QStringList& headers,
                                const QString& filePath,
                                QString* error) const {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) *error = file.errorString();
        return false;
    }

    // UTF-8 BOM
    const char bom[] = {char(0xEF), char(0xBB), char(0xBF)};
    file.write(bom, 3);

    // Write header
    QStringList headerFields = headers;
    if (headerFields.isEmpty()) {
        // Default order per A1 + remaining as documented conceptually
        headerFields << u"Company"_s << u"Position"_s << u"Status"_s << u"Applied"_s
                     << u"Next step"_s << u"Salary"_s << u"Source"_s << u"Updated"_s
                     << u"URL"_s << u"Resume version"_s << u"Location"_s << u"Work mode"_s
                     << u"Salary min"_s << u"Salary max"_s << u"Currency"_s
                     << u"Next action"_s << u"Next action date"_s << u"Contact name"_s
                     << u"Contact email"_s << u"Notes"_s << u"Created at"_s << u"Updated at"_s;
    }
    file.write(headerFields.join(u","_s).toUtf8());
    file.write(u"\r\n"_s.toUtf8());

    // Write rows
    for (const auto& r : rows) {
        QStringList fields;
        fields << escapeField(r.company)
               << escapeField(r.position)
               << escapeField(r.statusLabel)
               << escapeField(r.applied)
               << escapeField(r.nextStepText)
               << escapeField(r.salaryText)
               << escapeField(r.source)
               << escapeField(r.updated)
               << escapeField(r.url)
               << escapeField(r.resumeVersion)
               << escapeField(r.location)
               << escapeField(r.workModeLabel)
               << escapeField(r.salaryMin)
               << escapeField(r.salaryMax)
               << escapeField(r.currency)
               << escapeField(r.nextAction)
               << escapeField(r.nextActionDate)
               << escapeField(r.contactName)
               << escapeField(r.contactEmail)
               << escapeField(r.notes)
               << escapeField(r.createdAt)
               << escapeField(r.updatedAt);
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
