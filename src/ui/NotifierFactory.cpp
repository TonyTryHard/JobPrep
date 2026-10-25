#include "ui/NotifierFactory.h"

#include "ui/InAppNotifier.h"
#include "ui/MainWindow.h"
#include "ui/TrayNotifier.h"

namespace JobPrep::Ui {

std::unique_ptr<Services::INotifier> makeNotifier(
    bool trayAvailable,
    MainWindow& window,
    std::function<void(const Services::Notification&)> clickHandler) {
    if (trayAvailable) {
        return std::make_unique<TrayNotifier>(window, std::move(clickHandler));
    }
    return std::make_unique<InAppNotifier>(window, window.themeManager(), std::move(clickHandler));
}

}  // namespace JobPrep::Ui
