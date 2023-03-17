#include <event2/event.h>
#include <plog/Log.h>

#include <cerrno>
#include <stdexcept>

#include "TristLib/Event.h"

#ifdef CONFIG_WITH_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

using namespace TristLib::Event;

/**
 * @brief Check whether this platform supports watchdogs.
 */
bool SystemWatchdog::IsSupported() noexcept {
#if defined(CONFIG_WITH_SYSTEMD)
    return true;
#else
    return false;
#endif
}



/**
 * @brief Initialize system watchdog
 *
 * Determine the watchdog period and set up general state. The watchdog handling is not actually
 * invoked until the `start()` method is called.
 *
 * @param loop Run loop responsible for kicking the watchdog
 */
SystemWatchdog::SystemWatchdog(const std::shared_ptr<RunLoop> &loop) {
#if defined(CONFIG_WITH_SYSTEMD)
    uint64_t usec{0};
    int err;

    err = sd_watchdog_enabled(0, &usec);

    // watchdog is enabled
    if(err > 0) {
        this->interval = std::chrono::microseconds(usec);
        this->enabled = true;
    }
    // success, but watchdog not required
    else if(!err) {
        this->enabled = false;
    }
    // oopsie!
    else {
        throw std::system_error(errno, std::generic_category(), "sd_watchdog_enabled");
    }
#else
    throw std::runtime_error("watchdog not supported on this platform");
#endif

    PLOG_DEBUG << "Watchdog is " << (this->enabled ? "enabled" : "disabled") << ", interval "
               << this->interval.count() << " µS";

    // create the thymer
    this->timer = std::make_shared<Timer>(loop, this->interval, [this](auto timer) {
        this->kick();
    }, true, false);
}

/**
 * @brief Clean up watchdog resources
 */
SystemWatchdog::~SystemWatchdog() {
    // TODO: should we check the watchdog was previously stopped?
}


/**
 * @brief Start watchdog handling
 */
void SystemWatchdog::start() {
    if(!this->enabled) {
        return;
    }

    // start the timer
    this->timer->restart();

    // start watchdog
#if defined(CONFIG_WITH_SYSTEMD)
    sd_notify(0, "READY=1");
#endif
}

/**
 * @brief Stop watchdog handling
 */
void SystemWatchdog::stop() {
    if(!this->enabled) {
        return;
    }

#if defined(CONFIG_WITH_SYSTEMD)
    sd_notify(0, "STOPPING=1");
#endif
}

/**
 * @brief Kick the watchdog
 */
void SystemWatchdog::kick() {
    if(!this->enabled) {
        return;
    }

#if defined(CONFIG_WITH_SYSTEMD)
    sd_notify(0, "WATCHDOG=1");
#endif
}
