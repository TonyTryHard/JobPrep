#pragma once

#include <memory>

namespace JobPrep::Services {
class SettingsService;
}

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::App {

/// Composition root container holding shared services and theme managers.
class AppContext {
public:
    AppContext();
    ~AppContext();

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;
    AppContext(AppContext&&) = delete;
    AppContext& operator=(AppContext&&) = delete;

    Services::SettingsService& settings() const { return *m_settings; }
    Ui::Theme::ThemeManager& themeManager() const { return *m_themeManager; }

private:
    std::unique_ptr<Services::SettingsService> m_settings;
    std::unique_ptr<Ui::Theme::ThemeManager> m_themeManager;
};

}  // namespace JobPrep::App
