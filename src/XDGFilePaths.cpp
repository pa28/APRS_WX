/**
 * @file XDGFilePaths.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-02-18
 */

#include "XDGFilePaths.h"
#include <cstdlib>
#include <sstream>
#include <iostream>

namespace xdg {

    XDGFilePaths::XDGFilePaths() {
        mHome = getenv("HOME");
        for (const auto &path : mEnvVars) {
            bool inEnvironment{false};
            std::string envValue;
            auto env = getenv(std::string{path.varName}.c_str());
            if (env) {
                envValue = std::string{env};
                inEnvironment = true;
            } else {
                envValue = path.defaultPath;
            }

            XDG_Path_Set pathSet{};
            std::stringstream strm{envValue};
            std::string value{};
            while (std::getline(strm, value, ':')) {
                if (!inEnvironment && path.homeRelative) {
                    std::filesystem::path pathValue{mHome};
                    pathValue.append(value);
                    pathSet.push_back(pathValue);
                } else {
                    std::filesystem::path pathValue{value};
                    pathSet.push_back(pathValue);
                }
            }
            mPaths[path.name] = pathSet;
        }
    }

    std::filesystem::path Environment::getenv_path(XDGFilePaths::XDG_Name name, const std::string &appName, bool create) {
        auto [found,path] = mFilePaths.findFilePath(name, appName);
        if (!found && create) {
            std::filesystem::create_directories(path);
        }
        return path;
    }

    Environment::Environment(bool daemonMode) {
        if (!daemonMode)
            mHomeDirectory = std::string{getenv("HOME")};
        std::filesystem::path procExec{"/proc"};
        procExec.append("self").append("exe");

        if (std::filesystem::is_symlink(procExec)) {
            mAppName = std::filesystem::read_symlink(procExec).filename().string();

            if (!daemonMode) {
                mDataHome = getenv_path(XDGFilePaths::XDG_DATA_HOME, mAppName, true);
                mConfigHome = getenv_path(XDGFilePaths::XDG_CONFIG_HOME, mAppName, true);
                mCacheHome = getenv_path(XDGFilePaths::XDG_CACHE_HOME, mAppName, true);
            }
            mAppResources = getenv_path(XDGFilePaths::XDG_DATA_DIRS, mAppName, false);
            mLibResources = getenv_path(XDGFilePaths::XDG_DATA_DIRS, "Rose/resources", false);
        } else {
            std::cerr << StringCompositor('"', procExec, '"', " is not a symbolic link to application.\n");
            return;
        }
    }

}
