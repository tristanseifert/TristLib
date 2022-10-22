#ifndef TRISTLIB_EVENT_SIGNAL_H
#define TRISTLIB_EVENT_SIGNAL_H

#include <signal.h>

#include <array>
#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace TristLib::Event {
class RunLoop;

/**
 * @brief Signal event source
 *
 * Invokes a callback any time any of the specified signals are fired.
 *
 * @note Only a single run loop may receive signal events, due to an internal limitation in
 *       libevent. Adding signal handlers to multiple concurrent run loops will result in only one
 *       of the run loops (which is undefined) receiving signal events.
 */
class Signal {
    public:
        /// Default Ctrl+C equivalent signals (to quit the program)
        constexpr static const std::array<int, 3> kQuitEvents{{SIGINT, SIGTERM, SIGHUP}};

    public:
        Signal(const std::shared_ptr<RunLoop> &loop, int signal,
                const std::function<void(int)> &callback);
        Signal(const std::shared_ptr<RunLoop> &loop, std::span<const int> signals,
                const std::function<void(int)> &callback);
        ~Signal();

    private:
        void addEvent(const std::shared_ptr<RunLoop> &, const int);

    private:
        /// Underlying signal events
        std::vector<struct event *> events;
        /// Function to invoke on signal
        std::function<void(int)> callback;
};
}

#endif
