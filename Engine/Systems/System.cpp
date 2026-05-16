#include "gnspch.h"
#include "System.h"

#include "../Utils/Time.h"

SystemScopeTimer::SystemScopeTimer(TimerData& timer_data)
        : startTime(std::chrono::steady_clock::now()), timerData(timer_data)
{
}

SystemScopeTimer::~SystemScopeTimer()
{
    const auto endTime = std::chrono::steady_clock::now();

    const double elapsedMs =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();

    timerData.timeMs = elapsedMs;

    constexpr double smoothing = 0.01; // lower = smoother, higher = more responsive

    if (!timerData.hasTimingSample)
    {
        timerData.smoothedTimeMs = elapsedMs;
        timerData.hasTimingSample = true;
    }
    else
    {
        timerData.smoothedTimeMs =
            timerData.smoothedTimeMs * (1.0 - smoothing) +
            elapsedMs * smoothing;
    }
}
