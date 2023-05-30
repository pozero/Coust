#include "pch.h"

#include "utils/TimeStep.h"

namespace coust {

TimeStep::TimeStep(time_t const& start, time_t const& end) noexcept
    : m_delta_millisec(
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count()) {
}

float TimeStep::to_second() const noexcept {
    return float(m_delta_millisec) * 0.001f;
}

float TimeStep::to_milli_second() const noexcept {
    return float(m_delta_millisec);
}

}  // namespace coust
