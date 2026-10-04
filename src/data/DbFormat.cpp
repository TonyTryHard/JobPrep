#include "data/DbFormat.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data::DbFormat {

QDateTime now() {
    QDateTime current = QDateTime::currentDateTime();
    current.setMSecsSinceEpoch(0);
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

}  // namespace JobPrep::Data::DbFormat
