#ifndef TRISTLIB_EVENT_FLAG_H
#define TRISTLIB_EVENT_FLAG_H

#include <functional>
#include <memory>

struct event;

namespace TristLib::Event {
class RunLoop;

/**
 * @brief Manually signalled event
 *
 * Flags are events that are signalled manually, usually by another thread. They can be used to
 * synchronize different event loops.
 */
class Flag {
    public:
        /// Callback invoked when the flag is signalled
        typedef typename std::function<void(Flag *)> SignalCallback;

    public:
        Flag(const std::shared_ptr<RunLoop> &loop);
        ~Flag();

        void signal();

        /**
         * @brief Set event callback
         *
         * @param newCallback New callback to be invoked when the event is signalled
         */
        inline void setCallback(const SignalCallback &newCallback) {
            this->callback = newCallback;
        }

        /**
         * @brief Return the underlying libevent object
         */
        inline auto getEvent() const {
            return this->event;
        }

    private:
        /// event added to the event loop
        struct event *event{nullptr};

        /// Callback to invoke when the event is triggered
        SignalCallback callback;
};
}

#endif
