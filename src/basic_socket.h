//
// Created by Richard Buckley on 2021-08-28.
//

#pragma once

#include <iostream>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <future>
#include <unistd.h>
#include <fcntl.h>
#include <list>
#include <utility>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>

namespace sockets {
    using namespace std;

    /**
     * @brief The type of a socket
     */
    enum class SocketType {
        SockUnknown,    ///< Socket type is not known.
        SockListen,     ///< Socket is a listening or server socket
        SockConnect,    ///< Socket is a connecting or client socket
        SockAccept,     ///< Socket is an accepted connection
    };


    /**
     * @brief How the socket should be shutdown
     */
    enum SocketHow {
        SHUT_RD = 0,    ///< Further reception disabled
        SHUT_WR = 1,    ///< Further transmission disabled
        SHUT_RDWR = 2,  ///< Further reception and transmission disabled
    };


    class basic_socket {
    protected:
        string peer_host,       ///< The user provided peer host name or address.
                peer_port;      ///< The user provided peer port or service name.

        int af_type;            ///< The address family of the socket

        SocketType socket_type;     ///< The type of socket

        int sock_fd,            ///< The socket file descriptor
                status;         ///< Status of some called messages

        struct sockaddr_storage peer_addr;  ///< Storage of the peer address used to connect
        socklen_t peer_len;                 ///< The length of the peer address storage

        [[maybe_unused]] basic_socket(string host, string port) :
                peer_host{std::move(host)},
                peer_port{std::move(port)},
                af_type{AF_UNSPEC},
                socket_type{SocketType::SockUnknown},
                sock_fd{-1},
                status{0},
                peer_addr{},
                peer_len{sizeof(peer_addr)} {}

    public:

        ~basic_socket() {
            close();
        }

        /**
         * @brief Create a socket object to hold an accepted connection.
         */
        basic_socket(int fd,                     ///< The accepted connection file descriptor
                     struct sockaddr *addr,      ///< The peer address
                     socklen_t len               ///< The size of the peer address
        ) :
                peer_host{},
                peer_port{},
                af_type{addr->sa_family},
                socket_type{SocketType::SockAccept},
                sock_fd{fd},
                status{},
                peer_addr{},
                peer_len{} {
            memcpy(&peer_addr, addr, len);
            peer_len = len;
        }

        basic_socket &operator=(const basic_socket &) = delete;

        basic_socket &operator=(basic_socket &&other) noexcept {
            peer_host = std::move(other.peer_host);
            peer_port = std::move(other.peer_port);
            sock_fd = other.sock_fd;
            other.sock_fd = -1;
            af_type = other.af_type;
            other.af_type = AF_UNSPEC;
            peer_len = other.peer_len;
            memcpy(&peer_addr, &other.peer_addr, peer_len);
            return *this;
        }

        /**
         * @brief Generate a peer address string for the socket.
         * @param flags Flags that will be passed to getnameinfo()
         * @return a string with the form <host>:<service>
         * @details For Sockets of type SockListen the 'peer' is the hostname of the interface the
         * Socket is listening to. For other types it is the hostname of the remote machine.
         */
        string getPeerName(unsigned int flags = NI_NOFQDN | NI_NUMERICSERV) {
            string result;
            char hbuf[NI_MAXHOST];
            char sbuf[NI_MAXSERV];
            if (getnameinfo((const sockaddr *) &peer_addr, peer_len,
                            hbuf, sizeof(hbuf),
                            sbuf, sizeof(sbuf),
                            static_cast<int>(flags)) == 0) {
                result = string{hbuf} + ':' + sbuf;
            }

            return result;
        }


        /**
         * @brief Get the socket file descriptor
         * @return the socket file descriptor
         */
        [[nodiscard]] int fd() const {
            return sock_fd;
        }


        /**
         * @brief Uses fcntl(2) to set or clearflags on the socket file descriptor.
         * @param set When true set the specified flags, otherwise clear.
         * @param flags An or mask of flags to set or clear
         * @return -1 on error, 0 on success, errno is set to indicate the error encountered.
         */
        [[nodiscard]] int socketFlags(bool set, int flags) const {
            if (sock_fd < 0) {
                errno = EBADF;
                return -1;
            }

            int oflags = fcntl(sock_fd, F_GETFL);
            if (oflags < 0)
                return -1;

            if (fcntl(sock_fd, F_SETFL, (set ? oflags | flags : oflags & (~flags))))
                return -1;

            return 0;
        }


        /**
         * @brief Uses fcntl(2) to set or clear FD_CLOEXEC flag
         * @param close When true set the flag, otherwise clear
         * @return -1 on error, 0 on success, errno is set to indicate the error encountered.
         */
        [[nodiscard]] int closeOnExec(bool close) const {
            if (sock_fd < 0) {
                errno = EBADF;
                return -1;
            }

            int oflags = fcntl(sock_fd, F_GETFD);
            if (oflags < 0)
                return -1;

            if (fcntl(sock_fd, F_SETFD, (close ? oflags | FD_CLOEXEC : oflags & (~FD_CLOEXEC))))
                return -1;

            return 0;
        }


        /**
         * @brief Close a socket and set the internal file descriptor to -1;
         * @return the return value from ::close(2)
         */
        int close() {
            if (sock_fd >= 0) {
                int r = ::close(sock_fd);
                sock_fd = -1;
                return r;
            }

            return 0;
        }


        /**
         * @brief Shutdown a socket
         * @param how One of SHUT_RD, SHUT_WR or SHUT_RDWR
         * @return the return value from ::shutdown(2)
         */
        [[maybe_unused]] [[nodiscard]] int shutdown(SocketHow how) const {
            return ::shutdown(sock_fd, how);
        }


        /**
         * @brief Get the type of the socket
         * @return A SocketType value
         */
        [[nodiscard]] SocketType socketType() const { return socket_type; }


        /**
         * @brief Determine if the socket is open
         * @return true if open
         */
        explicit operator bool() const { return sock_fd >= 0; }


        /**
         * @brief Get the last set status return value for the socket
         * @return an integer status value
         */
        [[maybe_unused]] [[nodiscard]] int getStatus() const { return status; }


        /**
         * @brief Set a status value returned by a function called on the socket
         * @param s an integer status value
         */
        [[maybe_unused]] void setStatus(int s) { status = s; }

    };

    class local_socket : public basic_socket {
    protected:
        bool mNoDelay{false};

    public:
        local_socket(int fd, struct sockaddr *addr, socklen_t addr_len) : basic_socket(fd, addr, addr_len) {}

        local_socket(const string &host, const string &port, bool noDelay = false) :
                basic_socket{host, port} {
            mNoDelay = noDelay;
        }


        /**
         * @brief Complete a socket as a connection or client socket
         * @tparam AiFamilyPrefs A template parameter pack for a list of AF families
         * @param familyPrefs A list of AF family values AF_INET6, AF_INET, AF_UNSPEC
         * @return the socket fd or -1 on error
         */
        template<typename... AiFamilyPrefs>
        int connect(AiFamilyPrefs... familyPrefs) {
            list<int> prefsList{};
            (prefsList.push_back(familyPrefs), ...);

            findPeerInfo(::connect, prefsList);

            socket_type = SocketType::SockConnect;

            if (mNoDelay) {
                int on{1};
                status = setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (void *) &on, sizeof(on));
                if (status) {
                    cerr << "Set TCP_NODELAY " << status << ' ' << strerror(errno) << '\n';
                }
            }
            return sock_fd;
        }


        /**
         * @brief Complet a socket as a listen or server socket
         * @tparam AiFamilyPrefs A template parameter pack for a list of AF families
         * @param backlog the parameter passed to listen(2) as backlog
         * @param familyPrefs A list of AF family values AF_INET6, AF_INET, AF_UNSPEC
         * @return -1 on error, 0 on success
         * @details Finds a connection specification that allows a socket to be created
         * and bound preferring the provided address family preference, if any. If the
         * socket fd is successfully created this method also calls:
         *  - socketFlags(true, O_NONBLOCK)
         *  - closeOnExec(true)
         */
        template<typename... AiFamilyPrefs>
        [[maybe_unused]] int listen(int backlog, AiFamilyPrefs... familyPrefs) {
            int socketFlagSet = O_NONBLOCK;
            bool closeExec = true;

            list<int> prefsList{};
            (prefsList.push_back(familyPrefs), ...);

            findPeerInfo(::bind, prefsList);

            if (sock_fd >= 0) {
                ::listen(sock_fd, backlog);

                socket_type = SocketType::SockListen;

                /**
                 * Allow socket reuse.
                 */
                int on{1};
                status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));

                return std::min(socketFlags(true, socketFlagSet), closeOnExec(closeExec));
            }

            return -1;
        }


    protected:
        /**
         * @brief This method does the bulk of the work to complete realization of a socket.
         * @param bind_connect either ::bind() for a server or ::connect() for a client.
         * @param ai_family_preference the preferred address family AF_INET, AF_INET6 or AF_UNSPEC
         */
        void findPeerInfo(int (*bind_connect)(int, const struct sockaddr *, socklen_t),
                          list<int> &ai_family_preference) {

            struct addrinfo hints{};
            struct addrinfo *peer_info{nullptr};
            memset(&hints, 0, sizeof(hints));

            hints.ai_flags = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;

            if ((status = getaddrinfo((peer_host.length() ? peer_host.c_str() : nullptr),
                                      peer_port.c_str(), &hints, &peer_info))) {
                freeaddrinfo(peer_info);
                peer_info = nullptr;
                memset(&hints, 0, sizeof(hints));
                throw logic_error(string{"getaddrinfo error: "} + gai_strerror(status));
            }

            sock_fd = -1;
            socket_type = SocketType::SockUnknown;

            // Loop over preferences
            for (auto pref: ai_family_preference) {
                // And each discovered connection possibility
                for (struct addrinfo *peer = peer_info; peer != nullptr; peer = peer->ai_next) {
                    // Apply preference
                    if (pref == AF_UNSPEC || pref == peer->ai_family) {

                        // Create a compatible socket
                        sock_fd = ::socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);

                        /**
                         * Either bind or connect the socket. On error collect the message,
                         * close the socket and set it to error condition. Try the next
                         * connection or return error.
                         */
                        if (bind_connect(sock_fd, peer->ai_addr, peer->ai_addrlen)) {
                            ::close(sock_fd);
                            sock_fd = -1;
                        } else {
                            /**
                             * Store the selected peer address
                             */
                            memcpy(&peer_addr, peer->ai_addr, peer->ai_addrlen);
                            peer_len = peer->ai_addrlen;
                            af_type = peer->ai_family;

                            break;
                        }
                    }
                }

                if (sock_fd >= 0)
                    break;
            }

            freeaddrinfo(peer_info);
            peer_info = nullptr;
        }


        /**
         * @brief Call select the socket iff it is a listen socket
         * @tparam Duration template parameter for duration of timeout
         * @param duration A std::chrono duration to use as the timeout
         * @return the value returned from ::select()
         */
        template <typename Duration>
        int select(Duration &&duration) {
            struct timeval timeout{};

            std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
            timeout.tv_sec = sec.count();
            timeout.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(duration - sec).count();
            return select(&timeout);
        }

        /**
         * @brief Call select the socket iff it is a listen socket
         * @param selectClients What operations to select the clients on
         * @param timeout A timeout value in a timeval struct or nullptr for no timeout
         * @return the value returned from ::select()
         */
        int select(struct timeval *timeout = nullptr) {
            fd_set rd_set{};

            if (socket_type != SocketType::SockListen && socket_type != SocketType::SockConnect) {
                throw logic_error("Listen on a non-listening socket.");
            }

            FD_ZERO(&rd_set);
            FD_SET( sock_fd, &rd_set );
            int n = sock_fd + 1;

            return ::select(n, &rd_set, nullptr, nullptr, timeout);
        }


        /**
         * @brief Accept a connection request on a listener socket, add the accepted connection
         * to the connection list.
         * @param acceptFlags Socket flags to set on except see accept4()
         * @return
         */
        template <class Socket_t>
        [[maybe_unused]] unique_ptr<Socket_t> accept(int acceptFlags = SOCK_CLOEXEC) {
            if (socketType() == SocketType::SockListen) {
                struct sockaddr_storage client_addr{};
                socklen_t length = sizeof(client_addr);

                int clientfd = ::accept4(sock_fd, (struct sockaddr *) &client_addr, &length, acceptFlags);
                return std::make_unique<Socket_t>(clientfd, (struct sockaddr *) &client_addr, length);
            }

            throw logic_error("Accept on a non-listening socket.");
        }
    };
}
