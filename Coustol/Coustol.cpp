#include "Coust.h"

namespace coust {

std::unique_ptr<Application> create_application() {
    return std::make_unique<Application>();
}

}  // namespace coust
