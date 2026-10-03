#pragma once

#include "ui/PageBase.h"

namespace JobPrep::Ui::Pages {

/// Study preparation board placeholder page.
class StudyPage : public PageBase {
    Q_OBJECT

public:
    explicit StudyPage(QWidget* parent = nullptr);
    ~StudyPage() override = default;
};

}  // namespace JobPrep::Ui::Pages
