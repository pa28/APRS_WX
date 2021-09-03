/**
 * @file ConfigFile.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-09-02
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <utility>

/**
 * @class ConfigFile
 * @brief
 */
class ConfigFile {
public:
    enum Status {
        OK,
        NO_FILE,
        OPEN_FAIL,
    };

    struct Spec {
        std::string_view mKey{};
        std::size_t mIdx{};
    };

protected:
    std::filesystem::path mConfigFilePath{};
    std::ifstream mIstrm{};
    std::string_view mCallsign{};
    std::string_view mPasscode{};
    double mLatitude{0};
    double mLongitude{0};
    long mRadius{0};

    std::string::size_type matchKey(std::string::iterator first, std::string::iterator last, const std::string_view& key);


public:
    ConfigFile() = delete;

    explicit ConfigFile(std::filesystem::path configFilePath) : mConfigFilePath(std::move(configFilePath)) {}

    Status open();

    Status process(const std::vector<ConfigFile::Spec>& configSpecs, const std::function<void(std::size_t, const std::string_view&)>& callback);

    void close();

};

