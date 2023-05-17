#include "pch.h"

#include "utils/Compiler.h"
#include "event/Event.h"
#include "core/Application.h"

namespace coust {
namespace event_bus {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wunused-template")
template <detail::IsEvent E>
static void publish(E&& event) {
    Application::get_instance().on_event(event);
}
WARNING_POP

};  // namespace event_bus

}  // namespace coust
