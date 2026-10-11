#pragma once

#include <QColor>
#include <QIcon>
#include <QString>
#include "domain/Enums.h"

namespace JobPrep::Ui::Theme {

/// Maps an `AgendaKind` to its color, display label and icon. Lives in the UI layer
/// so `models/` never has to know about colors or `Tokens` (SPEC §7.6, Amendment C5).
class AgendaKindStyle {
public:
    static QColor color(Domain::AgendaKind kind);
    static QString label(Domain::AgendaKind kind);
    static QString iconName(Domain::AgendaKind kind);
    static QIcon icon(Domain::AgendaKind kind, const QColor& tint);
};

}  // namespace JobPrep::Ui::Theme
