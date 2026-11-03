#pragma once

namespace JobPrep::App {

/// Ensures Qt resources (.qrc) are registered, even when linked statically
/// into binaries without an AppContext.
/// Safe to call multiple times.
void ensureResources();

}  // namespace JobPrep::App
