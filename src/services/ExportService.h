#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace JobPrep::Services {

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
};

class ExportService {
public:
    ExportService() = default;
    bool exportToFile(const QList<ExportRow>& rows,
                      const QStringList& headers,
                      const QString& filePath,
                      QString* error = nullptr) const;
};

}  // namespace JobPrep::Services
