#include <sys/socket.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

#include "TristLib/Event.h"

using namespace TristLib::Event;

/**
 * @brief Create a new socket that's not yet connected
 *
 * You must connect the socket before attempting to read or write to it.
 *
 * @param loop Run loop to add the event source to
 * @parm type Type of socket to create
 */
Socket::Socket(const std::shared_ptr<RunLoop> &loop, const int type) {
    if(type != SOCK_STREAM) {
        // TODO: support other socket types
        throw std::invalid_argument("invalid type");
    }

    auto bev = bufferevent_socket_new(loop->getEvBase(), -1,
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    if(!bev) {
        throw std::runtime_error("failed to create bufevent");
    }

    this->installCallbacks(bev);
    this->event = bev;
}

/**
 * @brief Create a new SSL socket that's not yet connected
 *
 * You must connect the socket before attempting to read or write to it.
 *
 * @param loop Run loop to add the event source to
 * @param ssl SSL context to install
 * @parm type Type of socket to create
 */
Socket::Socket(const std::shared_ptr<RunLoop> &loop, SSL *sslCtx, const int type) {
    if(type != SOCK_STREAM) {
        // TODO: support other socket types
        throw std::invalid_argument("invalid type");
    }

    auto bev = bufferevent_openssl_socket_new(loop->getEvBase(), -1, sslCtx,
            BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    if(!bev) {
        throw std::runtime_error("failed to create SSL bufevent");
    }

    this->event = bev;

    this->installCallbacks(bev);
}


/**
 * @brief Create a new socket event source, with an existing socket
 *
 * @param loop Run loop to add the event source to
 * @param fd Socket to wrap
 * @param closeFd When set, the socket is closed automatically on deallocation
 */
Socket::Socket(const std::shared_ptr<RunLoop> &loop, const int fd, const bool closeFd) : fd(fd) {
    // make the socket non-blocking
    int err = evutil_make_socket_nonblocking(fd);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "evutil_make_socket_nonblocking");
    }

    // create the event
    auto bev = bufferevent_socket_new(loop->getEvBase(), fd,
            (closeFd ? BEV_OPT_CLOSE_ON_FREE : 0) | BEV_OPT_DEFER_CALLBACKS);
    if(!bev) {
        throw std::runtime_error("failed to create bufevent");
    }

    this->event = bev;

    this->installCallbacks(bev);
}

/**
 * @brief Create a new socket event source, with an existing socket, which uses TLS
 *
 * Creates a socket that can perform SSL communication without any application intervention. The
 * SSL handshake is performed automatically.
 *
 * @param loop Run loop to add the event source to
 * @param fd Socket to wrap (assumed to be accepted already)
 * @param sslCtx OpenSSL client context for the socket
 * @param closeFd When set, the socket is closed automatically on deallocation
 */
Socket::Socket(const std::shared_ptr<RunLoop> &loop, const int fd, SSL *sslCtx,
        const bool closeFd) : fd(fd) {
    // make the socket non-blocking
    int err = evutil_make_socket_nonblocking(fd);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "evutil_make_socket_nonblocking");
    }

    // create the event
    auto bev = bufferevent_openssl_socket_new(loop->getEvBase(), fd, sslCtx,
            BUFFEREVENT_SSL_ACCEPTING,
            (closeFd ? BEV_OPT_CLOSE_ON_FREE : 0) | BEV_OPT_DEFER_CALLBACKS);
    if(!bev) {
        throw std::runtime_error("failed to create SSL bufevent");
    }

    this->event = bev;

    this->installCallbacks(bev);
}

/**
 * @brief Install socket callbacks
 *
 * @param event Buffer event to install callbacks on
 */
void Socket::installCallbacks(struct bufferevent *event) {
    bufferevent_setcb(event, [](auto bev, auto ctx) {
        auto sock = reinterpret_cast<Socket *>(ctx);
        if(sock->readCallback.has_value()) {
            (*sock->readCallback)(sock);
        }
    }, [](auto bev, auto ctx) {
        auto sock = reinterpret_cast<Socket *>(ctx);
        if(sock->writeCallback.has_value()) {
            (*sock->writeCallback)(sock);
        }
    }, [](auto bev, auto what, auto ctx) {
        auto sock = reinterpret_cast<Socket *>(ctx);
        sock->handleEvents(what);
    }, this);
}

/**
 * @brief Release socket and associated resources
 *
 * If requested during allocation, we will close the socket here as well.
 */
Socket::~Socket() {
    if(this->event) {
        bufferevent_free(this->event);
    }
}



/**
 * @brief Connect to the specified host
 *
 * @param hostname Host to connect to
 * @param port Port to connect to
 */
void Socket::connect(const std::string_view &hostname, const uint16_t port) {
    int err = bufferevent_socket_connect_hostname(this->event, nullptr, AF_UNSPEC, hostname.data(),
            port);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "bufferevent_socket_connect_hostname");
    }
}

/**
 * @brief Update the socket's water mark
 *
 * The watermark defines the minimum and maximum boundaries for reads and writes; the exact
 * semantics differ based on the type:
 *
 * - Reads: Callback is not invoked until at least `low` bytes are available; stop reading when
 *          `high` bytes are pending.
 * - Writes: Callback is invoked whenever less than `low` bytes are pending.
 *
 * @param read When set, update the read watermark; otherwise, update the write watermark
 * @param level A pair of (low, high) watermark. Specify SIZE_MAX for no limit.
 */
void Socket::setWatermark(const bool read, const std::pair<size_t, size_t> level) {
    // convert values
    auto [low, high] = level;
    if(low == SIZE_MAX) {
        low = EV_RATE_LIMIT_MAX;
    }
    if(high == SIZE_MAX) {
        high = EV_RATE_LIMIT_MAX;
    }

    // apply it
    bufferevent_setwatermark(this->event, read ? EV_READ : EV_WRITE, low, high);
}

/**
 * @brief Enable event reporting
 *
 * @param read Whether read events are enabled
 * @param write Whether write events are enabled
 *
 * @remark If an event is not explicitly enabled, its previous state is maintained; it will _not_
 *         be disabled.
 */
void Socket::enableEvents(const bool read, const bool write) {
    short flags{0};
    if(read) {
        flags |= EV_READ;
    }
    if(write) {
        flags |= EV_WRITE;
    }

    int err = bufferevent_enable(this->event, flags);
    if(err == -1) {
        throw std::runtime_error("bufferevent_enable failed");
    }
}

/**
 * @brief Disable event reporting
 *
 * @param read Whether read events are disabled
 * @param write Whether write events are disabled
 *
 * @remark If an event is not explicitly disabled, its previous state is maintained; it will _not_
 *         be enabled.
 */
void Socket::disableEvents(const bool read, const bool write) {
    short flags{0};
    if(read) {
        flags |= EV_READ;
    }
    if(write) {
        flags |= EV_WRITE;
    }

    int err = bufferevent_disable(this->event, flags);
    if(err == -1) {
        throw std::runtime_error("bufferevent_disable failed");
    }
}

/**
 * @brief Handle socket events
 *
 * Translates the libevent flags to our internal flags.
 */
void Socket::handleEvents(const size_t flags) {
    // ensure we've a handler
    if(!this->eventCallback.has_value()) {
        return;
    }

    // convert the flags and invoke callback
    Event what{Event::None};

    if(flags & BEV_EVENT_READING) {
        what = static_cast<Event>(what | Event::ReadError);
    }
    if(flags & BEV_EVENT_WRITING) {
        what = static_cast<Event>(what | Event::WriteError);
    }
    if(flags & BEV_EVENT_EOF) {
        what = static_cast<Event>(what | Event::EndOfFile);
    }
    if(flags & BEV_EVENT_ERROR) {
        what = static_cast<Event>(what | Event::UnrecoverableError);
    }
    if(flags & BEV_EVENT_TIMEOUT) {
        what = static_cast<Event>(what | Event::Timeout);
    }
    if(flags & BEV_EVENT_CONNECTED) {
        what = static_cast<Event>(what | Event::Connected);
    }

    (*this->eventCallback)(this, what);
}

/**
 * @brief Read out pending data
 *
 * Drain as much data as possible from the underlying buffer.
 *
 * @param readData Buffer to receive the read out data
 *
 * @return Total number of bytes read
 */
size_t Socket::read(std::span<std::byte> readData) {
    return bufferevent_read(this->event, reinterpret_cast<void *>(readData.data()),
            readData.size());
}

/**
 * @brief Write data to socket
 *
 * Appends data to the write queue of the socket.
 *
 * @param writeData Data to write to the socket
 */
size_t Socket::write(std::span<const std::byte> writeData) {
    return bufferevent_write(this->event, reinterpret_cast<const void *>(writeData.data()),
            writeData.size());
}

/**
 * @brief Get the most recent OpenSSL error
 *
 * If the bufferevent was created with an SSL context, retrieve the most recent SSL error.
 *
 * @return OpenSSL error from error stack, or 0 if none.
 *
 * @remark Behavior is undefined if the socket wasn't created with an SSL context.
 */
unsigned long Socket::getSslError() {
    return bufferevent_get_openssl_error(this->event);
}

/**
 * @brief Flush the socket's write buffers
 */
void Socket::flushWriteBuffer() {
    int err = bufferevent_flush(this->event, EV_WRITE, BEV_FLUSH);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "bufferevent_flush failed");
    }
}

/**
 * @brief Manually increment the reference count
 *
 * This is useful when mixing our C++ wrappers with other LibEvent code, such as the HTTP library,
 * which takes ownership over some objects and may lead to double frees.
 */
void Socket::incref() {
    bufferevent_incref(this->event);
}
