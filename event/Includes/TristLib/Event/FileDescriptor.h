#ifndef TRISTLIB_EVENT_FILEDESCRIPTOR_H
#define TRISTLIB_EVENT_FILEDESCRIPTOR_H

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <utility>

struct event;

namespace TristLib::Event {
class RunLoop;

/**
 * @brief Wrapper for a file descriptor
 *
 * A basic observer on a file descriptor that's triggered whenever the descriptor becomes readable,
 * writeable, error state, or a combination thereof.
 */
class FileDescriptor {
    public:
        /// Callback type for any events
        typedef typename std::function<void(FileDescriptor *)> Callback;

    public:
        FileDescriptor(const std::shared_ptr<RunLoop> &loop, const int fd);
        ~FileDescriptor();

        void incref();

        void enableEvents(const bool read, const bool write);
        void disableEvents(const bool read, const bool write);

        /**
         * @brief Set read callback
         *
         * @param newCallback New callback to be invoked whenever data is ready to be read
         */
        inline void setReadCallback(const Callback &newCallback) {
            this->readCallback = newCallback;
        }
        /**
         * @brief Set write callback
         *
         * @param newCallback New callback to be invoked whenever write data can be accepted
         */
        inline void setWriteCallback(const Callback &newCallback) {
            this->writeCallback = newCallback;
        }
        /**
         * @brief Set event callback
         *
         * @param newCallback New callback to be invoked for any error event
         */
        inline void setEventCallback(const Callback &newCallback) {
            this->eventCallback = newCallback;
        }

        /**
         * @brief Return the underlying libevent object
         */
        inline auto getEvent() {
            return this->event;
        }

    private:
        void handleEvents(const short);

    private:
        /// Underlying file descriptor
        const int fd{-1};
        /// Event object for the socket
        struct ::event *event{nullptr};

        /// Whether read and write events are enabled
        bool readEnabled{false}, writeEnabled{false};

        /// Read callback
        std::optional<Callback> readCallback;
        /// Write callback
        std::optional<Callback> writeCallback;
        /// Event callback
        std::optional<Callback> eventCallback;

};
}

#endif

