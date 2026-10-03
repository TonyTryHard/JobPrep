#pragma once

#include "ui/PageBase.h"

namespace JobPrep::Ui::Pages {

/// Dashboard placeholder page.
class HomePage : public PageBase {
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);
    ~HomePage() override = default;
};

}  // namespace JobPrep::Ui::Pages
