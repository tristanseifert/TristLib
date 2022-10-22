#ifndef SUPPORT_EVENTLOOP_H
#define SUPPORT_EVENTLOOP_H

#include <sys/signal.h>

#include <array>
#include <cstddef>
#include <memory>

namespace TristLib::Event {
class Source;

/**
 * @brief Event loop
 *
 * This sets up a libevent-based loop, which can have various sources attached to it.
 */
class RunLoop: public std::enable_shared_from_this<RunLoop> {
    public:
        RunLoop();
        ~RunLoop();

        /**
         * @brief Arm the event loop for execution
         *
         * This doesn't really do anything other than set it as the active event loop for the
         * calling thread.
         */
        inline void arm() {
            this->activate();
        }

        void run();
        void interrupt();

        /**
         * @brief Get libevent main loop
         */
        inline auto getEvBase() {
            return this->evbase;
        }

        /**
         * @brief Get the current thread's event loop
         *
         * Return the event loop that most recently executed on this thread. If no event loop
         * exists, it will return `nullptr`.
         */
        inline static std::shared_ptr<RunLoop> Current() {
            return gCurrentRunLoop.lock();
        }

    private:
        /**
         * @brief Mark this event loop as the calling thread's active loop
         */
        inline void activate() {
            gCurrentRunLoop = this->shared_from_this();
        }

    private:
        static thread_local std::weak_ptr<RunLoop> gCurrentRunLoop;

        /// libevent main loop
        struct event_base *evbase{nullptr};
};
}

#endif
