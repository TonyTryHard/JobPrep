#include "app/AppContext.h"
#include "services/SettingsService.h"
#include "ui/theme/ThemeManager.h"

static void initializeResources() {
    Q_INIT_RESOURCE(resources);
}

namespace JobPrep::App {

AppContext::AppContext() {
    initializeResources();
    m_settings = std::make_unique<Services::SettingsService>();
    m_themeManager = std::make_unique<Ui::Theme::ThemeManager>(*m_settings);
}

AppContext::~AppContext() = default;

}  // namespace JobPrep::App
