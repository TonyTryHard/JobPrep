#pragma once

#include <QColor>
#include <QString>
#include "domain/Enums.h"

namespace JobPrep::Ui::Theme {

/// Centralized status colors and user-facing labels per SPEC §7.6.
class StatusStyle {
public:
    static QColor color(Domain::ApplicationStatus status);
    static QString label(Domain::ApplicationStatus status);

    static QColor color(Domain::TopicStatus status);
    static QString label(Domain::TopicStatus status);

    static QColor color(Domain::Priority priority);
    static QString label(Domain::Priority priority);
};

}  // namespace JobPrep::Ui::Theme
