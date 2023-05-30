#pragma once

#include <chrono>

namespace coust {

class TimeStep {
public:
    TimeStep() noexcept = default;
    TimeStep(TimeStep&) noexcept = default;
    TimeStep(TimeStep const&) noexcept = default;
    TimeStep& operator=(TimeStep&) noexcept = default;
    TimeStep& operator=(TimeStep const&) noexcept = default;

public:
    using clock_t = std::chrono::steady_clock;
    using time_t = clock_t::time_point;

public:
    TimeStep(time_t const& start, time_t const& end) noexcept;

    float to_second() const noexcept;

    float to_milli_second() const noexcept;

public:
    int64_t m_delta_millisec;
};

}  // namespace coust
