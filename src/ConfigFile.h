/**
 * @file ConfigFile.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-09-02
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <utility>
#include <charconv>
#include <cstring>
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

        template<typename IndexType>
        Spec(const std::string_view key, IndexType idx) : mKey(key), mIdx(static_cast<std::size_t>(idx)) {}
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

    template<typename T>
    std::optional<T> safeConvert(const std::string_view &stringView) {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Type not supported.");

        if (stringView.empty())
            return std::nullopt;

        if constexpr (std::is_integral_v<T>) {
            // Use simpler (and safer?) std::from_chars() for integers because if is available.
            T value;
            auto[ptr, ec] = std::from_chars(stringView.data(), stringView.data() + stringView.length(), value);
            if (ec == std::errc() && ptr == stringView.data() + stringView.length())
                return value;
            else
                return std::nullopt;
        } else if constexpr (std::is_floating_point_v<T>) {
            // Use more cumbersome std::strtod() for doubles.
            static constexpr std::size_t BufferSize = 64;
            static char buffer[BufferSize];

            if (stringView.length() > BufferSize - 1)
                throw std::range_error("String to long to convert.");

            char *endPtr{nullptr};
            std::memset(buffer, 0, BufferSize);
            std::strncpy(buffer, stringView.data(), stringView.length());
            std::optional<T> value = std::strtod(buffer, &endPtr);
            if ((endPtr >= buffer) && (static_cast<unsigned long>(endPtr - buffer) < stringView.length()))
                value = std::nullopt;
            return value;
        }
    }

    static char nullFilter(char c) { return c; }

    static bool isalnum(char c) { return ::isalnum(c) != 0;}

    static bool isdigit(char c) { return ::isdigit(c) != 0;}

    static char toupper(char c) { return static_cast<char>(::toupper(c)); }

    static std::optional<std::string> parseText(const std::string_view &text, const std::function<bool(char)>& valid,
                                                const std::function<char(char)>& filter = ConfigFile::nullFilter) {
        std::string result{};
        for (auto c : text) {
            if (valid(c)) {
                auto f = filter(c);
                result.append(1, filter(c));
            } else {
                return std::nullopt;
            }
        }
        return result;
    }

};

