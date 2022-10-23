#ifndef TRISTLIB_EVENT_LISTENSOCKET_H
#define TRISTLIB_EVENT_LISTENSOCKET_H

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

#include <sys/socket.h>

namespace TristLib::Event {
class RunLoop;

/**
 * @brief Socket listen event source
 *
 * Waits for clients to connect to the monitored socket; invokes a callback for every new client
 * and any error conditions.
 */
class ListenSocket {
    public:
        /// Accept callback
        using AcceptCallback = std::function<void(ListenSocket *)>;

        /// Maximum number of pending clients to accept
        constexpr static const size_t kListenBacklog{10};

    public:
        ListenSocket(const std::shared_ptr<RunLoop> &loop, const AcceptCallback &callback,
                const int fd, const bool closeFd = true);
        ListenSocket(const std::shared_ptr<RunLoop> &loop, const AcceptCallback &callback,
                const std::filesystem::path &fsPath, const bool unlinkOld, const int type);

        ~ListenSocket();

        /**
         * @brief Get the underlying file descriptor
         */
        constexpr inline auto getFd() const {
            return this->fd;
        }

        int accept();

    private:
        void makeEvent(const std::shared_ptr<RunLoop> &);
        void listen();

        static int CreateSocket(const std::filesystem::path &, const bool, const int);
        static void MakeSocketNonblocking(const int);

    private:
        /// Callback to invoke whenever a new client is pending
        const AcceptCallback callback;
        /// Underlying file descriptor for the socket
        const int fd{-1};
        /// Whether to close the file descriptor when deallocating
        const bool closeFd{true};

        /// libevent event handle for this socket
        struct event *event{nullptr};
};
}

#endif
