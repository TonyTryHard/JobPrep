#pragma once

#include "ui/PageBase.h"

namespace JobPrep::Ui::Pages {

/// Calendar and agenda view placeholder page.
class CalendarPage : public PageBase {
    Q_OBJECT

public:
    explicit CalendarPage(QWidget* parent = nullptr);
    ~CalendarPage() override = default;
};

}  // namespace JobPrep::Ui::Pages
