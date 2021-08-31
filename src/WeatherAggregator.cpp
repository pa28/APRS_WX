/**
 * @file WeatherAggregator.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#include "APRS_Packet.h"
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
            for (auto &item : WeatherItemList) {
                std::optional<double> value;
                auto idx = static_cast<std::size_t>(item.wxSym);
                value = report.second->mWeatherValue[idx];

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

#if 0
        std::optional<double> temperature{}, relHumidity{};
        for (auto &item : WeatherItemList) {
            if (item.wxFlag != 'l') {
                auto idx = static_cast<std::size_t>(item.wxSym);
                if (mValueAggregate[idx].has_value()) {
                    auto value = mValueAggregate[idx].value() / mHannAggregate[idx].value();
                    if (item.wxSym == WxSym::Temperature)
                        temperature = value;
                    if (item.wxSym == WxSym::Humidity)
                        relHumidity = value;
                    if (item.units == Units::Fahrenheit)
                        value = FahrenheitToCelsius(value);
                    else if (item.units == Units::inch_100)
                        value = value * 25.4;
                    else if (item.units == Units::MPH)
                        value = value * 1.60934;
                    std::cout << '\t' << std::setw(9) << item.prefix << ": "
                              << fixed << setprecision(item.precision) << value
                              << ' ' << item.suffix << '\n';
                }
            }
        }

        if (temperature.has_value() && relHumidity.has_value()) {
            auto celsius = FahrenheitToCelsius(temperature.value());
            auto dewPoint = celsius - ((100. - relHumidity.value()) / 5.);
            auto e = 6.11 * exp(5417.7530 * ((1. / 273.16) - (1. / (dewPoint + 273.15))));
            auto h = 0.5555 * (e - 10.0);
            auto humidex = celsius + h;
            cout << setprecision(0)
                 << '\t' << setw(11) << "Dew Point: " << dewPoint << '\n'
                 << '\t' << setw(11) << "Humidex: " << humidex << '\n';
        }
#else
        cout.unsetf(ios::fixed | ios::scientific);
        cout << setprecision(6);

        std::string prefix = "aprs_wx,aggregate ";

        std::optional<double> temperature{}, relHumidity{};
        for (auto &item : WeatherItemList) {
            if (item.wxFlag != 'l') {
                auto idx = static_cast<std::size_t>(item.wxSym);
                if (mValueAggregate[idx].has_value()) {
                    auto value = mValueAggregate[idx].value() / mHannAggregate[idx].value();
                    if (item.wxSym == WxSym::Temperature)
                        temperature = value;
                    if (item.wxSym == WxSym::Humidity)
                        relHumidity = value;
                    if (item.units == Units::Fahrenheit)
                        value = FahrenheitToCelsius(value);
                    else if (item.units == Units::inch_100)
                        value = value * 25.4;
                    else if (item.units == Units::MPH)
                        value = value * 1.60934;
                    cout << prefix << item.dbName << '=' << value << '\n';
                }
            }
        }

        if (temperature.has_value() && relHumidity.has_value()) {
            auto celsius = FahrenheitToCelsius(temperature.value());
            auto dewPoint = celsius - ((100. - relHumidity.value()) / 5.);
            auto e = 6.11 * exp(5417.7530 * ((1. / 273.16) - (1. / (dewPoint + 273.15))));
            auto h = 0.5555 * (e - 10.0);
            auto humidex = celsius + h;
            cout << prefix << "DewPt=" << dewPoint << '\n';
            cout << prefix << "Humidex=" << humidex << '\n';
        }
#endif
    }
}
