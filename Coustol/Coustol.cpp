#include "Coust.h"

namespace coust {

memory::unique_ptr<Application, DefaultAlloc> create_application() {
    return memory::allocate_unique<Application>(get_default_alloc());
}

}  // namespace coust
