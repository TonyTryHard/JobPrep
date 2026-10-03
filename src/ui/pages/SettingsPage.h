#pragma once

#include "ui/PageBase.h"

class QComboBox;
class QSpinBox;
class QCheckBox;

namespace JobPrep::Services {
class SettingsService;
}

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Pages {

/// Application settings page with theme and preference controls.
class SettingsPage : public PageBase {
    Q_OBJECT

public:
    explicit SettingsPage(Services::SettingsService& settings,
                          Theme::ThemeManager& themeMgr,
                          QWidget* parent = nullptr);
    ~SettingsPage() override = default;

private:
    void setupUi();
    void loadSettings();

    Services::SettingsService& m_settings;
    Theme::ThemeManager& m_themeMgr;

    QComboBox* m_themeCombo{nullptr};
    QComboBox* m_accentCombo{nullptr};
    QSpinBox* m_studyGoalSpinBox{nullptr};
    QComboBox* m_weekStartCombo{nullptr};
    QCheckBox* m_closeToTrayCheck{nullptr};
};

}  // namespace JobPrep::Ui::Pages
