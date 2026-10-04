#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringView>
#include "domain/Structs.h"

namespace JobPrep::Data {

class Database;

/// Bookkeeping of already delivered reminders (SPEC §5 `reminders_sent`, §3.5 R2).
/// Keys look like `interview:<id>:<leadMinutes>` or `followup:<appId>:<yyyy-MM-dd>`.
class ReminderLogRepository : public QObject {
    Q_OBJECT

public:
    explicit ReminderLogRepository(Database& database, QObject* parent = nullptr);
    ~ReminderLogRepository() override;

    bool wasSent(QStringView key) const;

    /// Remembers `key`; a key already present keeps its original `sentAt`.
    bool markSent(const QString& key, const QDateTime& sentAt = QDateTime());
    bool remove(const QString& key);

    QList<QString> sentKeys() const;
    QString lastError() const;
    JobPrep::Data::Database& database() const;

signals:
    void changed();

private:
    Database& m_database;
};

}  // namespace JobPrep::Data
