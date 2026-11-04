#include "data/DbFormat.h"

#include <QTimeZone>

using namespace Qt::StringLiterals;

namespace JobPrep::Data::DbFormat {

const QDateTime kLegacyTimestampCutoffUtc{QDate(2013, 1, 1), QTime(0, 0, 0), QTimeZone::UTC};

QDateTime now() {
    QDateTime current = QDateTime::currentDateTime();
    current.setSecsSinceEpoch(current.toSecsSinceEpoch());
    return current;
}

QString toText(const QDate& date) {
    return date.toString(u"yyyy-MM-dd"_s);
}

QString toText(const QDateTime& dateTime) {
    return dateTime.toString(u"yyyy-MM-ddTHH:mm:ss"_s);
}

std::optional<QDate> dateFromText(QStringView text) {
    const QDate date = QDate::fromString(text, u"yyyy-MM-dd"_s);
    if (!date.isValid()) return std::nullopt;
    return date;
}

std::optional<QDateTime> dateTimeFromText(QStringView text) {
    const QDateTime dateTime = QDateTime::fromString(text, u"yyyy-MM-ddTHH:mm:ss"_s);
    if (!dateTime.isValid()) return std::nullopt;
    return dateTime;
}

std::optional<QDate> dateFromValue(const QVariant& value) {
    return dateFromText(value.toString());
}

std::optional<QDateTime> dateTimeFromValue(const QVariant& value) {
    return dateTimeFromText(value.toString());
}

bool isPlausibleTimestamp(const QDateTime& dateTime) {
    if (!dateTime.isValid()) {
        return false;
    }
    if (dateTime.timeSpec() == Qt::LocalTime) {
        return dateTime.date() >= kLegacyTimestampCutoffUtc.date();
    }
    return dateTime.toUTC() >= kLegacyTimestampCutoffUtc;
}

bool isPlausibleTimestamp(const QDate& date) {
    if (!date.isValid()) {
        return false;
    }
    return date >= kLegacyTimestampCutoffUtc.date();
}

QString formatStoredTimestamp(const QDateTime& dateTime) {
    if (!isPlausibleTimestamp(dateTime)) {
        return u"—"_s;
    }
    return toText(dateTime.date());
}

QString formatStoredTimestamp(const QDate& date) {
    if (!isPlausibleTimestamp(date)) {
        return u"—"_s;
    }
    return toText(date);
}

}  // namespace JobPrep::Data::DbFormat
