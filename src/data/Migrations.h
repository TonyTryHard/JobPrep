#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include "data/Database.h"

namespace JobPrep::Data {

/// One schema step. `statements` holds single statements because the SQLite driver
/// executes only the first statement of a multi-statement string.
struct Migration {
    int         version{0};
    QStringList statements;
};

/// All shipped migrations, ordered by version and never modified after release.
QList<Migration> migrations();

int currentUserVersion(Database& database);

/// Applies every migration newer than `PRAGMA user_version`; on failure nothing is
/// applied and the version stays untouched.
bool runMigrations(Database& database, const QList<Migration>& list, QString* error);

}  // namespace JobPrep::Data
