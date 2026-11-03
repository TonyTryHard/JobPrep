#include "app/Resources.h"
#include <QtGlobal>

// Q_INIT_RESOURCE mangles the generated symbol with the enclosing namespace and
// declares it with the enclosing linkage, while AUTORCC defines a global symbol,
// so this helper must sit at global scope with external linkage.
void initializeResourcesGlobal() {
    Q_INIT_RESOURCE(resources);
}

namespace JobPrep::App {

void ensureResources() {
    static const bool initialized = []() {
        initializeResourcesGlobal();
        return true;
    }();
    Q_UNUSED(initialized);
}

}  // namespace JobPrep::App
