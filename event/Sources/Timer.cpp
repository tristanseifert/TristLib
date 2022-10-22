#include <time.h>

#include <event2/event.h>

#include <cerrno>
#include <stdexcept>

#include "TristLib/Event.h"

using namespace TristLib::Event;

/**
 * @brief Initialize a timer
 *
 * @param loop Run loop to add the timer handler to
 * @param interval Timer interval (µS)
 * @param callback Method to invoke when timer expires
 * @param repeating When set, the timer is repeating
 * @param start Whether the timer is started immediately
 */
Timer::Timer(const std::shared_ptr<RunLoop> &loop, const std::chrono::microseconds interval,
        const std::function<void(Timer *)> &callback, const bool repeating, const bool start) :
    callback(callback), interval(interval) {
    this->ev = event_new(loop->getEvBase(), -1, repeating ? EV_PERSIST : 0,
            [](auto, auto, auto ctx) {
        auto timer = reinterpret_cast<Timer *>(ctx);
        timer->callback(timer);
    }, this);
    if(!this->ev) {
        throw std::runtime_error("failed to allocate timer event");
    }

    if(start) {
        this->restart();
    }
}

/**
 * @brief Clean up timer resources
 */
Timer::~Timer() {
    event_del(this->ev);
    event_free(this->ev);
}

/**
 * @brief Re-arm the timer
 *
 * Set up the timer to trigger again.
 *
 * @remark Should only be called for non-repeating timers
 */
void Timer::restart() {
    struct timeval tv{
        .tv_sec  = static_cast<time_t>(this->interval.count() / 1'000'000U),
        .tv_usec = static_cast<suseconds_t>(this->interval.count() % 1'000'000U),
    };

    evtimer_add(this->ev, &tv);
}

/**
 * @brief Cancel the timer
 */
void Timer::invalidate() {
    event_del(this->ev);
}
