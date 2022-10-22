#ifndef TRISTLIB_EVENT_TIMER_H
#define TRISTLIB_EVENT_TIMER_H

#include <chrono>
#include <functional>
#include <memory>

namespace TristLib::Event {
class RunLoop;

/**
 * @brief Run loop timer
 */
class Timer {
    public:
        Timer(const std::shared_ptr<RunLoop> &loop, const std::chrono::microseconds interval,
                const std::function<void(Timer *)> &callback, const bool repeating = false,
                const bool start = true);
        ~Timer();

        void restart();
        void invalidate();

    private:
        /// Timer event
        struct event *ev{nullptr};
        /// Function to invoke on signal
        std::function<void(Timer *)> callback;

        /// Timer interval
        const std::chrono::microseconds interval;
};
};

#endif
