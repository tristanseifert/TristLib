#ifndef TRISTLIB_EVENT_SYSTEMWATCHDOG_H
#define TRISTLIB_EVENT_SYSTEMWATCHDOG_H

#include <chrono>
#include <memory>

namespace TristLib::Event {
class RunLoop;
class Timer;

/**
 * @brief System watchdog
 *
 * Handles kicking a system-provided watchdog (such as that implemented by the job supervisor we're
 * running under) periodically. This is done by a timer added to the event loop.
 *
 * Currently, only systemd is supported.
 */
class SystemWatchdog {
    public:
        static bool IsSupported() noexcept;

        SystemWatchdog(const std::shared_ptr<RunLoop> &loop);
        ~SystemWatchdog();

        void start();
        void stop();

        void kick();

    private:
        /// Is the watchdog enabled?
        bool enabled{false};
        /// Watchdog interval
        std::chrono::microseconds interval;
        /// Timer to kick the watchdog
        std::shared_ptr<Timer> timer;
};
}

#endif
