#include <signal.h>
#include <unistd.h>

#include <event2/event.h>

#include <cerrno>
#include <stdexcept>

#include "TristLib/Event.h"

using namespace TristLib::Event;

/**
 * @brief Create a signal handler for a single signal
 *
 * @param loop Run loop to add the signal handler to
 * @param signal Signal to be triggered on
 * @param callback Handler to invoke when signal is triggered
 */
Signal::Signal(const std::shared_ptr<RunLoop> &loop, int signal,
        const std::function<void(int)> &callback) : callback(callback) {
    this->addEvent(loop, signal);
}

/**
 * @brief Create a signal handler for multiple signals
 *
 * @param loop Run loop to add the signal handler to
 * @param signals List of signals to handle
 * @param callback Handler to invoke when signal is triggered
 */
Signal::Signal(const std::shared_ptr<RunLoop> &loop, std::span<const int> signals,
        const std::function<void(int)> &callback) : callback(callback) {
    if(signals.empty()) {
        throw std::invalid_argument("signals list may not be empty");
    }

    for(const auto signal : signals) {
        this->addEvent(loop, signal);
    }
}

/**
 * @brief Clean up the signal handler
 */
Signal::~Signal() {
    for(auto ev : this->events) {
        event_del(ev);
        event_free(ev);
    }
}



/**
 * @brief Create a signal handler event
 *
 * Initialize a signal handler event
 *
 * @param signum Signal number
 */
void Signal::addEvent(const std::shared_ptr<RunLoop> &loop, const int signum) {
    auto ev = evsignal_new(loop->getEvBase(), signum, [](auto fd, auto what, auto ctx) {
        // TODO: is this working to pass signal number?
        reinterpret_cast<Signal *>(ctx)->callback(what);
    }, this);
    if(!ev) {
        throw std::runtime_error("failed to allocate signal event");
    }

    event_add(ev, nullptr);
}
