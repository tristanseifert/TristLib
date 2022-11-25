#ifndef TRISTLIB_EVENT_SOCKET_H
#define TRISTLIB_EVENT_SOCKET_H

#include <sys/socket.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <utility>

struct ssl_st;
struct bufferevent;

namespace TristLib::Event {
class RunLoop;

/**
 * @brief Wrapper for a client socket
 *
 * This wraps a libevent buffer event, which can trigger various callbacks whenever data is
 * available to read, write, or an error occurs.
 */
class Socket {
    public:
        /**
         * @brief Socket event types
         *
         * Event callbacks will receive one or more of these events (combined as a logical OR) in
         * their callbacks.
         */
        enum Event: size_t {
            None                                = 0,
            ReadError                           = (1 << 0),
            WriteError                          = (1 << 1),
            EndOfFile                           = (1 << 4),
            UnrecoverableError                  = (1 << 5),
            Timeout                             = (1 << 6),
            Connected                           = (1 << 7)
        };

        /// Callback type for read/write callbacks
        using DataCallback = std::function<void(Socket *)>;
        /// Callback type for events
        using EventCallback = std::function<void(Socket *, const Event)>;

    public:
        Socket(const std::shared_ptr<RunLoop> &loop, const int type = SOCK_STREAM);
        Socket(const std::shared_ptr<RunLoop> &loop, struct ssl_st /* SSL */ *sslCtx,
                const int type = SOCK_STREAM);

        Socket(const std::shared_ptr<RunLoop> &loop, const int fd, const bool closeFd = true);
        Socket(const std::shared_ptr<RunLoop> &loop, const int fd, struct ssl_st /* SSL */ *sslCtx,
                const bool closeFd = true);
        ~Socket();

        void connect(const std::string_view &hostname, const uint16_t port);

        size_t read(std::span<std::byte> readData);
        size_t write(std::span<const std::byte> writeData);

        unsigned long getSslError();

        void flushWriteBuffer();
        void incref();

        /**
         * @brief Update the write watermark
         *
         * @param mark A pair of low and high watermarks
         *
         * @seeAlso setWatermark
         */
        inline void setWriteWatermark(const std::pair<size_t, size_t> mark) {
            this->setWatermark(false, mark);
        }
        /**
         * @brief Update the read watermark
         *
         * @param mark A pair of low and high watermarks
         *
         * @seeAlso setWatermark
         */
        inline void setReadWatermark(const std::pair<size_t, size_t> mark) {
            this->setWatermark(true, mark);
        }
        void setWatermark(const bool read, const std::pair<size_t, size_t> level);

        void enableEvents(const bool read, const bool write);
        void disableEvents(const bool read, const bool write);

        /**
         * @brief Set read callback
         *
         * @param newCallback New callback to be invoked whenever data is ready to be read
         */
        inline void setReadCallback(const DataCallback &newCallback) {
            this->readCallback = newCallback;
        }
        /**
         * @brief Set write callback
         *
         * @param newCallback New callback to be invoked whenever write data can be accepted
         */
        inline void setWriteCallback(const DataCallback &newCallback) {
            this->writeCallback = newCallback;
        }
        /**
         * @brief Set event callback
         *
         * @param newCallback New callback to be invoked for any socket event
         */
        inline void setEventCallback(const EventCallback &newCallback) {
            this->eventCallback = newCallback;
        }

        /**
         * @brief Return the underlying libevent object
         */
        inline auto getEvent() {
            return this->event;
        }

    private:
        void installCallbacks(struct bufferevent *);
        void handleEvents(const size_t);

    private:
        /// Underlying file descriptor
        const int fd{-1};
        /// Bufferevent for the socket
        struct ::bufferevent *event{nullptr};

        /// Read callback
        std::optional<DataCallback> readCallback;
        /// Write callback
        std::optional<DataCallback> writeCallback;
        /// Event callback
        std::optional<EventCallback> eventCallback;

};
}

#endif
