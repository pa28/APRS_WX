/**
 * @file APRS_Packet.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#pragma once

#include <iomanip>
#include <iostream>
#include <ios>
#include <cmath>
#include <cstring>
#include <chrono>
#include <optional>
#include <stdexcept>
#include "APRS_IS.h"

namespace aprs {
    /// Convert radians to degrees.
    template<typename T>
    static constexpr T rad2deg(T r) noexcept {
        return 180. * (r / M_PI);
    }

    /// Convert degrees to radians.
    template<typename T>
    static constexpr T deg2rad(T d) noexcept {
        return M_PI * (d / 180.);
    }

    template<typename T>
    std::optional<T> safeConvert(const std::string &str) {
        static constexpr std::size_t BufferSize = 64;
        if (str.empty())
            return std::nullopt;
        if (str.length() > BufferSize - 1)
            throw std::range_error("String to long to convert.");
        char buffer[BufferSize];
        char *endPtr{nullptr};
        std::strncpy(buffer, str.c_str(), str.length());
        if constexpr (std::is_same_v<T, long>) {
            std::optional<T> value = std::strtol(buffer, &endPtr, 10);
            if (endPtr - buffer < str.length())
                value = std::nullopt;
            return value;
        } else if constexpr (std::is_same_v<T, double>) {
            std::optional<T> value = std::strtod(buffer, &endPtr);
            if (endPtr - buffer < str.length())
                value = std::nullopt;
            return value;
        } else {
            static_assert(std::is_same_v<T, long> || std::is_same_v<T, double>, "Type not supported.");
        }
    }

    enum class PacketStatus {
        None,
        AprsPacket,
        PositionPacket,
        WxPacket,
        DecodingError,
        ErrorLatitude,
        ErrorLongitude,
    };

    class APRS_Packet {
    public:
        PacketStatus mPacketStatus{PacketStatus::None};
        std::string mName{};
        char mSymTableId{}, mSymCode{};
        std::chrono::time_point<std::chrono::steady_clock> mTimePoint;

        APRS_Packet() {
            setPacketTime();
        }

        explicit APRS_Packet(PacketStatus packetStatus) : APRS_Packet() {
            mPacketStatus = packetStatus;
        }

        [[nodiscard]] PacketStatus status() const { return mPacketStatus; }

        void setPacketTime() {
            mTimePoint = std::chrono::steady_clock::now();
        }

        virtual std::ostream &printOn(std::ostream &strm) const;
    };

    class APRS_Position : public APRS_Packet {
    public:
        std::optional<double> mLat{}, mLon{}, mDistance{}, mBearing{}, mHannValue{};

        bool setBearingDistance(const APRS_Position &other);

        std::ostream &printOn(std::ostream &strm) const override;
    };

    enum class WxSym {
        WindDirection,
        WindSpeed,
        WindGust,
        Temperature,
        Humidity,
        RainHour,
        RainDay,
        RainMidnight,
        Pressure,
        Luminosity,
        LuminosityPlus,
    };

    enum class Units {
        Degrees,
        MPH,
        KPH,
        mPs,
        Fahrenheit,
        Celsius,
        inch_100,
        mm,
        Percent,
        hPa,
        wPSqm
    };

    struct WeatherItem {
        WxSym wxSym;
        char wxFlag;
        std::size_t digits;
        std::string_view prefix;
        std::string_view suffix;
        Units units;
    };

    static constexpr std::array<WeatherItem, 11>
            WeatherItemList{{
                                    {WxSym::WindDirection, 'c', 3, "Dir", "", Units::Degrees},
                                    {WxSym::WindSpeed, 's', 3, "Wind", "", Units::MPH},
                                    {WxSym::WindGust, 'g', 3, "Gust", "", Units::MPH},
                                    {WxSym::Temperature, 't', 3, "Temp", "", Units::Fahrenheit},
                                    {WxSym::Humidity,'h', 2, "Humid", "", Units::Percent},
                                    {WxSym::RainHour, 'r', 3, "Rain", "/hour", Units::inch_100},
                                    {WxSym::RainDay, 'p', 3, "Rain", "/day", Units::inch_100},
                                    {WxSym::RainMidnight, 'P', 3, "Rain", "since midniht", Units::inch_100},
                                    {WxSym::Pressure, 'b', 5, "BP", "", Units::hPa},
                                    {WxSym::Luminosity, 'l', 3, "Lumin", "", Units::wPSqm},
                                    {WxSym::LuminosityPlus, 'L', 3, "Lumin", "", Units::wPSqm}
                            }};

    class WeatherValueError : public std::runtime_error {
    public:
        explicit WeatherValueError(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    class APRS_WX_Report : public APRS_Position {
    public:
        std::string mDateTime{};
        std::array<std::optional<double>,11> mWeatherValue;

        void decodeWeatherValue(APRS_IS &aprs_is, WxSym);

        std::ostream &printOn(std::ostream &strm) const override;

    };
}

