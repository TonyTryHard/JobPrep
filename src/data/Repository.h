#pragma once

#include <QObject>
#include "data/Database.h"

namespace JobPrep::Data {

/// Base of every repository: owns the single `changed()` signal and the two rules all
/// writes follow (one transaction each, one signal after the outer commit).
class Repository : public QObject {
    Q_OBJECT

public:
    explicit Repository(QObject* parent = nullptr);
    ~Repository() override = default;

signals:
    void changed();

protected:
    bool commitAndNotify(Database& database, Database::Transaction& transaction);

    /// True when `found`; otherwise records "No <what> with id <id>" and returns false.
    /// Writes addressed by id run this *before* opening a transaction: a row that is
    /// simply gone is "nothing to do" and must not poison a unit of work a service
    /// opened around them.
    bool acceptExisting(Database& database, bool found, const QString& what, int id) const;
};

}  // namespace JobPrep::Data