#include "ui/theme/AgendaKindStyle.h"
#include <QCoreApplication>
#include "ui/theme/IconProvider.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Theme {

QColor AgendaKindStyle::color(Domain::AgendaKind kind) {
    switch (kind) {
        case Domain::AgendaKind::Interview:
            return QColor(u"#F59E0B"_s);
        case Domain::AgendaKind::FollowUp:
            return QColor(u"#3B82F6"_s);
        case Domain::AgendaKind::StudyTarget:
            return QColor(u"#10B981"_s);
    }
    return QColor(u"#3B82F6"_s);
}

QString AgendaKindStyle::label(Domain::AgendaKind kind) {
    switch (kind) {
        case Domain::AgendaKind::Interview:
            return QCoreApplication::translate("AgendaKindStyle", "Interview");
        case Domain::AgendaKind::FollowUp:
            return QCoreApplication::translate("AgendaKindStyle", "Follow-up");
        case Domain::AgendaKind::StudyTarget:
            return QCoreApplication::translate("AgendaKindStyle", "Study target");
    }
    return QString();
}

QString AgendaKindStyle::iconName(Domain::AgendaKind kind) {
    switch (kind) {
        case Domain::AgendaKind::Interview:
            return u"agenda-interview"_s;
        case Domain::AgendaKind::FollowUp:
            return u"agenda-followup"_s;
        case Domain::AgendaKind::StudyTarget:
            return u"agenda-target"_s;
    }
    return u"agenda-interview"_s;
}

QIcon AgendaKindStyle::icon(Domain::AgendaKind kind, const QColor& tint) {
    return IconProvider::icon(iconName(kind), tint);
}

}  // namespace JobPrep::Ui::Theme
