#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <event2/event.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

#include "TristLib/Event.h"

using namespace TristLib::Event;



/**
 * @brief Initialize with existing socket
 *
 * Create a new listening socket event source that uses an existing socket.
 *
 * @param loop Run loop to install the event source on
 * @param callback Callback to invoke on pending client
 * @param fd File descriptor for a socket to receive clients on
 * @param closeFd When set, we'll close the file descriptor on release.
 *
 * @remark The specified socket must already be bound, but it _must not_ be listening already. It
 *         will be made non-blocking as part of this call.
 */
ListenSocket::ListenSocket(const std::shared_ptr<RunLoop> &loop, const AcceptCallback &callback,
        const int fd, const bool closeFd) : callback(callback), fd(fd), closeFd(closeFd) {
    // prepare socket
    MakeSocketNonblocking(fd);
    this->listen();

    // create the event
    this->makeEvent(loop);
}

/**
 * @brief Initialize an UNIX domain socket
 *
 * Create an UNIX domain socket (corresponding to a file, rather than an IP address/port combo)
 * and begins listening on it.
 *
 * @param loop Run loop to install the event source on
 * @param callback Callback to invoke on pending client
 * @param fsPath Path at which to create the listening socket
 * @param unlinkOld Whether the previous file at the location shall be unlinked
 * @param type Socket type (one of the `SOCK_` constants)
 */
ListenSocket::ListenSocket(const std::shared_ptr<RunLoop> &loop, const AcceptCallback &callback,
        const std::filesystem::path &fsPath, const bool unlinkOld, const int type) :
    callback(callback), fd(CreateSocket(fsPath, unlinkOld, type)) {
    // prepare it and create an event
    MakeSocketNonblocking(this->fd);
    this->listen();

    this->makeEvent(loop);
}

/**
 * @brief Deallocate the listening socket
 *
 * If requested during initialization (for custom fd's) the underlying file descriptor is closed.
 */
ListenSocket::~ListenSocket() {
    if(this->event) {
        event_del(this->event);
        event_free(this->event);
    }

    if(this->closeFd) {
        close(this->fd);
    }
}

/**
 * @brief Allocate an UNIX domain socket
 *
 * @param path Filesystme path to create the socket
 * @param unlinkOld Whether any previous file at the path is unlinked
 * @param type Socket type
 *
 * @return File descriptor initialized
 */
int ListenSocket::CreateSocket(const std::filesystem::path &path, const bool unlinkOld,
        const int type) {
    int err, fd;

    // delete previous file, if any
    if(unlinkOld) {
        err = unlink(path.native().c_str());
        if(err == -1 && errno != ENOENT) {
            throw std::system_error(errno, std::generic_category(), "unlink old socket");
        }
    }

    // create the socket
    fd = socket(AF_UNIX, type, 0);
    if(fd == -1) {
        throw std::system_error(errno, std::generic_category(), "create socket");
    }

    // bind the socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.native().c_str(), sizeof(addr.sun_path) - 1);

    err = bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if(err == -1) {
        close(fd);
        throw std::system_error(errno, std::generic_category(), "bind socket");
    }

    return fd;
}

/**
 * @brief Create an event to trigger on accept
 *
 * Create a new event source that triggers when a client is pending on the socket. It will be added
 * immediately to the run loop and can fire events immediately as well.
 *
 * @param loop Run loop to add the source to
 */
void ListenSocket::makeEvent(const std::shared_ptr<RunLoop> &loop) {
    this->event = event_new(loop->getEvBase(), this->fd, (EV_READ | EV_PERSIST),
            [](auto fd, auto what, auto ctx) {
        auto me = reinterpret_cast<ListenSocket *>(ctx);
        me->callback(me);
    }, this);
    if(!this->event) {
        throw std::runtime_error("failed to allocate listen event");
    }

    event_add(this->event, nullptr);
}

/**
 * @brief Make the specified socket non-blocking
 *
 * @int fd File descriptor for a socket to modify
 */
void ListenSocket::MakeSocketNonblocking(const int fd) {
    int err;

    err = fcntl(fd, F_GETFL);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "get socket flags");
    }

    err = fcntl(fd, F_SETFL, err | O_NONBLOCK);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "set socket flags");
    }
}

/**
 * @brief Start listening for connections
 *
 * Set up our socket to begin listening for connections.
 */
void ListenSocket::listen() {
    int err = ::listen(this->fd, kListenBacklog);
    if(err == -1) {
        throw std::system_error(errno, std::generic_category(), "start listening on socket");
    }
}

/**
 * @brief Accept a pending client connection
 *
 * @return File descriptor for the accepted client
 */
int ListenSocket::accept() {
    // TODO: get sockaddr for source address?
    int fd = ::accept(this->fd, nullptr, nullptr);
    if(fd == -1) {
        throw std::system_error(errno, std::generic_category(), "accept");
    }

    return fd;
}
