#pragma once

#include <functional>
#include <memory>
#include "services/INotifier.h"

namespace JobPrep::Ui {
class MainWindow;

/// Instantiates concrete INotifier based on system tray availability (SPEC §8, Amendment C5).
std::unique_ptr<Services::INotifier> makeNotifier(
    bool trayAvailable,
    MainWindow& window,
    std::function<void(const Services::Notification&)> clickHandler = {});

}  // namespace JobPrep::Ui
