/**
 * @file WeatherAggregator.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#include "WeatherAggregator.h"

namespace aprs {

    void WeatherAggregator::clearAggregateData() {
        for (auto &value : mValueAggregate)
            value = std::nullopt;
        for (auto &hann : mHannAggregate)
            hann = std::nullopt;

        auto now = std::chrono::steady_clock::now();

        for (auto it = begin(); it != end();) {
            std::chrono::duration<double> diff = now - it->second->mTimePoint;
            if (diff.count() > 3600.)
                it = erase(it);
            else
                ++it;
        }
    }

    void WeatherAggregator::aggregateData() {
        clearAggregateData();

        cout << "\tAgregate from " << size() << " stations.\n";
        for (const auto &report : (*this)) {
            auto hann = report.second->mHannValue.value();
            for (std::size_t idx = 0; idx < WeatherParam; ++idx) {
                std::optional<double> value;
                switch (static_cast<WxSym>(idx)) {
                    case WxSym::WindDirection:
                        if (report.second->mWindDir.has_value())
                            value = static_cast<double>(report.second->mWindDir.value());
                        break;
                    case WxSym::WindSpeed:
                        value = report.second->mWindSpeed;
                        break;
                    case WxSym::WindGust:
                        value = report.second->mWindGust;
                        break;
                    case WxSym::Temperature:
                        value = report.second->mTemperature;
                        break;
                    case WxSym::Humidity:
                        if (report.second->mHumidity.has_value())
                            value = static_cast<double>(report.second->mHumidity.value());
                        break;
                    case WxSym::RainHour:
                        value = report.second->mRainHour;
                        break;
                    case WxSym::RainDay:
                        value = report.second->mRainDay;
                        break;
                    case WxSym::RainMidnight:
                        value = report.second->mRainMidnight;
                        break;
//                    case WxSym::Snow:
//                        if (report.second->mSnow.has_value())
//                            value = static_cast<double>(report.second->mSnow.value());
//                        break;
                    case WxSym::Pressure:
                        value = report.second->mBarometricPressure;
                        break;
                    case WxSym::Luminosity:
                        if (report.second->mLuminosity.has_value())
                            value = static_cast<double>(report.second->mLuminosity.value());
                        break;
                }

                if (value.has_value()) {
                    if (mValueAggregate[idx].has_value()) {
                        mValueAggregate[idx].value() += value.value() * hann;
                        mHannAggregate[idx].value() += hann;
                    } else {
                        mValueAggregate[idx] = value.value() * hann;
                        mHannAggregate[idx] = hann;
                    }
                }
            }
        }

        std::optional<double> temperature{}, relHumidity{};
        for (std::size_t idx = 0; idx < WeatherParam; ++idx) {
            std::optional<double> value;
            switch (static_cast<WxSym>(idx)) {
                case WxSym::WindDirection:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tWindDir:      ";
                    break;
                case WxSym::WindSpeed:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tWindSpeed:    ";
                    break;
                case WxSym::WindGust:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tWindGust:     ";
                    break;
                case WxSym::Temperature:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tTemperature:  ";
                    break;
                case WxSym::Humidity:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tHumidity:     ";
                    break;
                case WxSym::RainHour:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tRainHour:     ";
                    break;
                case WxSym::RainDay:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tRainDay:      ";
                    break;
                case WxSym::RainMidnight:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tRainMidnight: ";
                    break;
//                case WxSym::Snow:
//                    if (mValueAggregate[idx].has_value())
//                        cout << "\tSnow:         ";
//                    break;
                case WxSym::Pressure:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tB Pressure:   ";
                    break;
                case WxSym::Luminosity:
                    if (mValueAggregate[idx].has_value())
                        cout << "\tLuminosity:   ";
                    break;
            }
            if (mValueAggregate[idx].has_value()) {
                cout << setprecision(1) << fixed
                     << mValueAggregate[idx].value() / mHannAggregate[idx].value() << '\n';

                if (idx == static_cast<std::size_t>(WxSym::Temperature))
                    temperature = mValueAggregate[idx].value() / mHannAggregate[idx].value();
                else if (idx == static_cast<std::size_t>(WxSym::Humidity))
                    relHumidity = mValueAggregate[idx].value() / mHannAggregate[idx].value();
            }
        }

        if (temperature.has_value() && relHumidity.has_value()) {
            auto dewPoint = temperature.value() - ((100. - relHumidity.value()) / 5.);
            auto e = 6.11 * exp(5417.7530 * ((1. / 273.16) - (1. / (dewPoint + 273.15))));
            auto h = 0.5555 * (e - 10.0);
            auto humidex = temperature.value() + h;
            cout << "\tDew Point:    " << dewPoint << '\n'
                 << "\tHumidex:      " << humidex << '\n';
        }
    }
}
