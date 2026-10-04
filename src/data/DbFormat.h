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

/// Current local time truncated to whole seconds, so that values written and read
/// back through the text columns compare equal.
QDateTime now();

QString toText(const QDate& date);
QString toText(const QDateTime& dateTime);

std::optional<QDate> dateFromText(QStringView text);
std::optional<QDateTime> dateTimeFromText(QStringView text);

std::optional<QDate> dateFromValue(const QVariant& value);
std::optional<QDateTime> dateTimeFromValue(const QVariant& value);

}  // namespace DbFormat

}  // namespace JobPrep::Data
