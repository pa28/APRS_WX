//
// Created by richard on 2021-09-05.
//

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <memory>
#include <climits>
#include <string_view>

namespace unixstd {

    /**
     * @class Hostname
     * @brief Get access to system host name as a std::string_view.
     * @details Hostname provides a singleton class which gets the system hostname on creation and holds
     * the hostname buffer and a member std::string_view for the life of the program. The host name is
     * retrieved using gethostname(2), if this call returns a non-zero value errno is copied into mErrno
     * and mHostname is not set.
     */
    class Hostname {
    protected:
        char hostname[HOST_NAME_MAX + 1]{};     ///< Local storage of the hostname.
        int mErrno{0};                          ///< Local storage of any error code causted by gethostname.

        std::string_view mHostname{};           ///< Local std::string_view reference to hostname.

        /**
         * @brief Protected constructor, gets the host name and sets local error number storage.
         */
        explicit Hostname() {
            memset(hostname, 0, HOST_NAME_MAX + 1);
            if (gethostname(hostname, HOST_NAME_MAX + 1)) {
                mErrno = errno;
            } else {
                mHostname = hostname;
            }
        }

    public:
        ~Hostname() = default;

        Hostname(const Hostname &) = delete;

        Hostname(Hostname &&) = delete;

        Hostname& operator=(const Hostname &) = delete;

        Hostname& operator=(Hostname &&) = delete;

        /**
         * @brief Get a reference to the singleton Hostname.
         * @return A reference to Hostname.
         */
        static Hostname &getHostname() {
            static Hostname instance{};
            return instance;
        }

        /**
         * @brief Returns the status of Hostname.
         * @return true if hostname is valid.
         */
        explicit operator bool () const {
            return !mHostname.empty() && mErrno == 0;
        }

        /**
         * @brief Get the error number returned as a result of gethostname.
         * @return 0 if no error, the value of errno after construction otherwise.
         */
        [[nodiscard]] static int getError() {
            return getHostname().mErrno;
        }

        /**
         * @brief Get the std::string_view representation of the hostname.
         * @return std::string_view which will be empty on error.
         */
        [[nodiscard]] static std::string_view& name() {
            return getHostname().mHostname;
        }
    };
}
