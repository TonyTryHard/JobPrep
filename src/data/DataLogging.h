#pragma once

#include <QLoggingCategory>

/// Shared logging category for the data layer (jobprep.data).
/// Defined in Database.cpp; declared here for use in all data/*.cpp files.
Q_DECLARE_LOGGING_CATEGORY(lcData)
