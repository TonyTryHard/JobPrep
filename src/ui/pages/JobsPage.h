#pragma once

#include "ui/PageBase.h"

namespace JobPrep::Ui::Pages {

/// Job applications tracker placeholder page.
class JobsPage : public PageBase {
    Q_OBJECT

public:
    explicit JobsPage(QWidget* parent = nullptr);
    ~JobsPage() override = default;
};

}  // namespace JobPrep::Ui::Pages
