#pragma once

#include <optional>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QStringView>
#include <QVariant>

namespace JobPrep::Data {

/// ISO-8601 text conversions used by every repository.
/// Date-only values are `yyyy-MM-dd`, date-times are local time without offset
/// (`yyyy-MM-ddTHH:mm:ss`), so plain string comparison is chronological (SPEC §8).
namespace DbFormat {

/// Cutoff for legacy timestamps (e.g. 1970 defaults before M6 D1 fix).
/// Any system timestamp earlier than 2013-01-01 UTC is considered legacy.
extern const QDateTime kLegacyTimestampCutoffUtc;

/// Current local time truncated to whole seconds, so that values written and read
/// back through the text columns compare equal.
QDateTime now();

QString toText(const QDate& date);
QString toText(const QDateTime& dateTime);

std::optional<QDate> dateFromText(QStringView text);
std::optional<QDateTime> dateTimeFromText(QStringView text);

std::optional<QDate> dateFromValue(const QVariant& value);
std::optional<QDateTime> dateTimeFromValue(const QVariant& value);

/// Returns true if the timestamp is on or after the 2013-01-01 UTC cutoff.
bool isPlausibleTimestamp(const QDateTime& dateTime);
bool isPlausibleTimestamp(const QDate& date);

/// Formats a stored system timestamp for UI presentation. Returns "—" (em dash)
/// if the value is earlier than the 2013 cutoff, or the date formatted as yyyy-MM-dd.
QString formatStoredTimestamp(const QDateTime& dateTime);
QString formatStoredTimestamp(const QDate& date);

}  // namespace DbFormat

}  // namespace JobPrep::Data
