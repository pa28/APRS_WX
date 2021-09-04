/**
 * @file XDGFilePaths.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-02-18
 */

#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace xdg {

/**
 * @class XDGFilePaths
 * @brief A class to determine and provide searching of XDG standard files paths.
 * @details https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */
    class XDGFilePaths {
    public:
        enum XDG_Name : std::size_t {
            XDG_DATA_HOME,
            XDG_CONFIG_HOME,
            XDG_DATA_DIRS,
            XDG_CONFIG_DIRS,
            XDG_CACHE_HOME,
            XDG_RUNTIME_DIR
        };

        struct XDG_Env_Spec {
            XDG_Name name;
            std::string_view varName;
            std::string_view defaultPath;
            bool homeRelative;
        };

        using XDG_Env_Var_List = std::array<XDG_Env_Spec, 6>;

        using XDG_Path_Set = std::vector<std::filesystem::path>;

        using XDG_Paths = std::map<XDG_Name, XDG_Path_Set>;

    protected:
        static constexpr XDG_Env_Var_List mEnvVars = {
                XDG_Env_Spec{XDG_DATA_HOME, "XDG_DATA_HOME", ".local/share", true},
                XDG_Env_Spec{XDG_CONFIG_HOME, "XDG_CONFIG_HOME", ".config", true},
                XDG_Env_Spec{XDG_DATA_DIRS, "XDG_DATA_DIRS", "/usr/local/share/:/usr/share/", false},
                XDG_Env_Spec{XDG_CONFIG_DIRS, "XDG_CONFIG_DIRS", "/etc/xdg", false},
                XDG_Env_Spec{XDG_CACHE_HOME, "XDG_CACHE_HOME", ".cache", true},
                XDG_Env_Spec{XDG_RUNTIME_DIR, "XDG_RUNTIME_DIR", "", false}
        };

        XDG_Paths mPaths{};     ///< All the XDG specified paths.

        std::string mHome;      ///< User's home directory.

    public:

        XDGFilePaths();

        /**
         * @brief Search for a relative path on one of the XDG standard locations.
         * @details If the relative path is found below one of the specified paths in the named location, the
         * return value is true with the full absolute path to the location. If not found the return value is
         * false with the full absolute path to the relative location in the preferred directory.
         * @tparam S The type of the relative path, implicitly convertible to a std::string.
         * @param name The XDG name
         * @param relativePath A path releative to an XDG location to search for.
         * @return A std::tuple<bool,std::filesystem::path>
         */
        template<typename S>
        std::tuple<bool,std::filesystem::path> findFilePath(XDG_Name name, const S &relativePath) {
            for (auto const &path : mPaths[name]) {
                auto tryPath{path};
                tryPath.append(relativePath);
                if (std::filesystem::exists(tryPath))
                    return std::make_tuple(true,tryPath);
            }

            auto preferredPath{mPaths[name].front()};
            preferredPath.append(relativePath);
            return std::make_tuple(false, preferredPath);
        }
    };

    /**
    * @brief Composite a pack of arguments that are streamable to a string.
    * @tparam Arg First argument type
    * @tparam Args Rest of the argument types
    * @param arg First argument
    * @param args Rest of the arguments
    * @return The composited string
    */
    template<typename Arg, typename... Args>
    std::string StringCompositor(Arg &&arg, Args &&... args) {
        std::stringstream out;
        out << std::forward<Arg>(arg);
        ((out << std::forward<Args>(args)), ...);
        return out.str();
    }

    class Environment {
    protected:
        explicit Environment(bool daemonMode);

        std::filesystem::path mHomeDirectory{};     ///< The user's home directory path
        std::filesystem::path mDataHome{};          ///< The user's XDG Data Home path
        std::filesystem::path mConfigHome{};        ///< The user's XDG Config Home path
        std::filesystem::path mCacheHome{};         ///< The user's XDG Cache Home path
        std::filesystem::path mAppResources;        ///< Resources installed with the application.

        /**
         * @brief Resources shared by applications using the library.
         * @details The content of this directory is maintained by the library developer. Any user data
         * placed or installed in this directory may be over written by updates.
         */
        std::filesystem::path mLibResources;
        XDGFilePaths mFilePaths{};                  ///< XDG Spec file paths.

        std::string mAppName;                       ///< Application name as provided by the system.

    public:
        ~Environment() = default;

        Environment(const Environment &) = delete;

        Environment(Environment &&) = delete;

        Environment& operator=(const Environment &) = delete;

        Environment& operator=(Environment &&) = delete;

        static Environment &getEnvironment(bool daemonMode = false) {
            static Environment instance{daemonMode};
            return instance;
        }

        [[nodiscard]] const std::string& appName() const { return mAppName; }

        [[nodiscard]] const std::filesystem::path& configHome() const { return mConfigHome; }

        [[nodiscard]] const std::filesystem::path& cacheHome() const { return mCacheHome; }

        [[nodiscard]] const std::filesystem::path& dataHome() const { return mDataHome; }

        [[nodiscard]] const std::filesystem::path& appResources() const { return mAppResources; }

        template<typename Source>
        std::filesystem::path appResourcesAppend(Source source) {
            auto path = mAppResources;
            return path.append(source);
        }

        /**
         * @brief Find the XDG directory for a specified application name.
         * @details If the path location does not exist, and create is set to true, it is created along with all
         * parent directories with permissions set to std::filesystem::perms::all modified by umask(2).
         * @param name The XDG Name.
         * @param appName The application name.
         * @param create Set to true if non-existent directories should be created.
         * @return
         */
        std::filesystem::path getenv_path(XDGFilePaths::XDG_Name name, const std::string &appName, bool create);
    };
}
