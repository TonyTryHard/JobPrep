#include "ui/theme/StatusStyle.h"
#include <QCoreApplication>

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Theme {

QColor StatusStyle::color(Domain::ApplicationStatus status) {
    switch (status) {
        case Domain::ApplicationStatus::Wishlist:
            return QColor(u"#8A93A6"_s);
        case Domain::ApplicationStatus::Applied:
            return QColor(u"#3B82F6"_s);
        case Domain::ApplicationStatus::HrScreen:
            return QColor(u"#8B5CF6"_s);
        case Domain::ApplicationStatus::Technical:
            return QColor(u"#F59E0B"_s);
        case Domain::ApplicationStatus::Final:
            return QColor(u"#F97316"_s);
        case Domain::ApplicationStatus::Offer:
            return QColor(u"#10B981"_s);
        case Domain::ApplicationStatus::Accepted:
            return QColor(u"#059669"_s);
        case Domain::ApplicationStatus::Rejected:
            return QColor(u"#EF4444"_s);
        case Domain::ApplicationStatus::Withdrawn:
            return QColor(u"#64748B"_s);
        case Domain::ApplicationStatus::Ghosted:
            return QColor(u"#94A3B8"_s);
    }
    return QColor(u"#8A93A6"_s);
}

QString StatusStyle::label(Domain::ApplicationStatus status) {
    switch (status) {
        case Domain::ApplicationStatus::Wishlist:
            return QCoreApplication::translate("StatusStyle", "Wishlist");
        case Domain::ApplicationStatus::Applied:
            return QCoreApplication::translate("StatusStyle", "Applied");
        case Domain::ApplicationStatus::HrScreen:
            return QCoreApplication::translate("StatusStyle", "HR Screen");
        case Domain::ApplicationStatus::Technical:
            return QCoreApplication::translate("StatusStyle", "Technical");
        case Domain::ApplicationStatus::Final:
            return QCoreApplication::translate("StatusStyle", "Final");
        case Domain::ApplicationStatus::Offer:
            return QCoreApplication::translate("StatusStyle", "Offer");
        case Domain::ApplicationStatus::Accepted:
            return QCoreApplication::translate("StatusStyle", "Accepted");
        case Domain::ApplicationStatus::Rejected:
            return QCoreApplication::translate("StatusStyle", "Rejected");
        case Domain::ApplicationStatus::Withdrawn:
            return QCoreApplication::translate("StatusStyle", "Withdrawn");
        case Domain::ApplicationStatus::Ghosted:
            return QCoreApplication::translate("StatusStyle", "Ghosted");
    }
    return QString();
}

QColor StatusStyle::color(Domain::TopicStatus status) {
    switch (status) {
        case Domain::TopicStatus::Backlog:
            return QColor(u"#8A93A6"_s);
        case Domain::TopicStatus::InProgress:
            return QColor(u"#3B82F6"_s);
        case Domain::TopicStatus::Review:
            return QColor(u"#F59E0B"_s);
        case Domain::TopicStatus::Done:
            return QColor(u"#10B981"_s);
    }
    return QColor(u"#8A93A6"_s);
}

QString StatusStyle::label(Domain::TopicStatus status) {
    switch (status) {
        case Domain::TopicStatus::Backlog:
            return QCoreApplication::translate("StatusStyle", "Backlog");
        case Domain::TopicStatus::InProgress:
            return QCoreApplication::translate("StatusStyle", "In Progress");
        case Domain::TopicStatus::Review:
            return QCoreApplication::translate("StatusStyle", "Review");
        case Domain::TopicStatus::Done:
            return QCoreApplication::translate("StatusStyle", "Done");
    }
    return QString();
}

QColor StatusStyle::color(Domain::Priority priority) {
    switch (priority) {
        case Domain::Priority::Low:
            return QColor(u"#10B981"_s);
        case Domain::Priority::Normal:
            return QColor(u"#3B82F6"_s);
        case Domain::Priority::High:
            return QColor(u"#EF4444"_s);
    }
    return QColor(u"#3B82F6"_s);
}

QString StatusStyle::label(Domain::Priority priority) {
    switch (priority) {
        case Domain::Priority::Low:
            return QCoreApplication::translate("StatusStyle", "Low");
        case Domain::Priority::Normal:
            return QCoreApplication::translate("StatusStyle", "Normal");
        case Domain::Priority::High:
            return QCoreApplication::translate("StatusStyle", "High");
    }
    return QString();
}

}  // namespace JobPrep::Ui::Theme
