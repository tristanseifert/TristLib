#include <event2/event.h>

#include <cerrno>
#include <stdexcept>

#include "TristLib/Event.h"

using namespace TristLib::Event;

/**
 * @brief Current event loop
 *
 * This is a thread-local that is set to the `this` value when we start executing a run loop.
 */
thread_local std::weak_ptr<RunLoop> RunLoop::gCurrentRunLoop;



/**
 * @brief Initialize the event loop
 */
RunLoop::RunLoop() {
    this->evbase = event_base_new();
    if(!this->evbase) {
        throw std::runtime_error("failed to allocate event_base");
    }
}

/**
 * @brief Release event loop resources
 *
 * @remark The event loop should be stopped when destroying.
 */
RunLoop::~RunLoop() {
    // TODO: could we check and remove any pending events?

    event_base_free(this->evbase);
}

/**
 * @brief Run event loop
 *
 * Process events on the event loop.
 *
 * @remark This will sit here basically forever; kicking of the watchdog is implemented by means
 *         of a timer callback that runs periodically.
 */
void RunLoop::run() {
    this->activate();

    event_base_dispatch(this->evbase);
}

/**
 * @brief Interrupt the run loop
 *
 * Cause the next invocation of the run loop to return.
 */
void RunLoop::interrupt() {
    event_base_loopbreak(this->evbase);
}
