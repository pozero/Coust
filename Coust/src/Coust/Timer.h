#pragma once

#include <chrono>

namespace Coust
{
    typedef std::chrono::steady_clock Clock_t;
    using Time_t = Clock_t::time_point;

    class TimeStep
    {
    public:
        TimeStep(const Time_t& start, const Time_t& end)
        {
            m_DeltaMiliSec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }

        float ToSecond() const
        {
            return float(m_DeltaMiliSec) * 0.001f;
        }

        float ToMiliSecond() const
        {
            return float(m_DeltaMiliSec);
        }

    private:
        TimeStep() = default;

    private:
        int64_t m_DeltaMiliSec;    // May be too much?
    };

    class Timer
    {
    public:
        Timer()
            : m_Start(Clock_t::now()) {}
        
        void Reset()
        {
            m_Start = Clock_t::now();
        }

        TimeStep GetTimeElapsed()
        {
            Time_t end = Clock_t::now();
            return TimeStep{m_Start, end};
        }

    private:
        Time_t m_Start;
    };
}