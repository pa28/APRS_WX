/**
 * @file WeatherAggregator.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#include <cmath>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
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
            // Delete any weather reports more than 90 minutes old.
            if (diff.count() > 5400.)
                it = erase(it);
            else
                ++it;
        }
    }

    std::ostream &WeatherAggregator::printInfluxFormat(ostream &strm, const std::string &prefix) const {
        std::optional<double> temperature{}, relHumidity{}, windGust{};
        for (auto &item : WeatherItemList) {
            if (item.wxFlag != 'l') {
                auto idx = static_cast<std::size_t>(item.wxSym);
                if (mValueAggregate[idx].has_value()) {
                    auto value = mValueAggregate[idx].value() / mHannAggregate[idx].value();
                    switch (item.units) {
                        case Units::inch_100:
                            if (value < 0.01)
                                value = 0.;
                            break;
                        default:
                            break;
                    }

                    // Apply conversion for temperature, length and speed.
                    if (item.units == Units::Fahrenheit)
                        value = FahrenheitToCelsius(value);
                    else if (item.units == Units::inch_100)
                        value = value * 25.4;
                    else if (item.units == Units::MPH)
                        value = value * 1.60934;

                    // Gather values needed for humidex and wind chill.
                    if (item.wxSym == WxSym::Temperature)
                        temperature = value;
                    else if (item.wxSym == WxSym::Humidity)
                        relHumidity = value;
                    else if (item.wxSym == WxSym::WindGust)
                        windGust = value;

                    // Add value to influx push.
                    strm << prefix << item.dbName << '=' << value << '\n';
                }
            }
        }

        // Compute dew point, humidex and wind chill.
        if (temperature.has_value() && relHumidity.has_value()) {
            // dew point and humidex.
            auto celsius = temperature.value();
            auto dewPoint = celsius - ((100. - relHumidity.value()) / 5.);
            auto e = 6.11 * exp(5417.7530 * ((1. / 273.16) - (1. / (dewPoint + 273.15))));
            auto h = 0.5555 * (e - 10.0);
            auto humidex = celsius + h;

            strm << prefix << "DewPt=" << dewPoint << '\n';
            strm << prefix << "Humidex=" << humidex << '\n';
        }

        if (temperature.has_value() && windGust.has_value()) {
            auto celsius = temperature.value();
            auto velocity = windGust.value();

            auto windChill = 13.12 + 0.6215 * celsius - 11.37 * pow(velocity,0.16) + 0.3965 * celsius * pow(velocity,0.16);

            strm << prefix << "WindChill=" << windChill << '\n';
        }

        return strm;
    }

    void WeatherAggregator::aggregateData() {
        clearAggregateData();

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

        cout.unsetf(ios::fixed | ios::scientific);
        cout << setprecision(6);
    }

    bool WeatherAggregator::pushToInflux(const string &host, bool tls, unsigned int port, const std::string &dataBase) {
        std::stringstream buildUrl{};
        std::stringstream postField{};

        buildUrl << (tls? "https" : "http") << "://" << host << ':' << port << "/write?db=" << dataBase;
//        std::cerr << "CurlPP URL: " << buildUrl.str() << '\n';

        printInfluxFormat(postField, "aggregate,call=VE3YSH ");
//        std::cerr << postField.str() << '\n';
        auto postData = postField.str();

        if (!postData.empty())
            try {
                cURLpp::Cleanup cleaner;
                cURLpp::Easy request;
                request.setOpt(new cURLpp::Options::Url(buildUrl.str()));
                request.setOpt(new curlpp::options::Verbose(false));

                std::list<std::string> header;
                header.emplace_back("Content-Type: application/octet-stream");
                request.setOpt(new curlpp::options::HttpHeader(header));

                request.setOpt(new curlpp::options::PostFieldSize(postData.length()));
                request.setOpt(new curlpp::options::PostFields(postData));

                request.perform();
            } catch (curlpp::LogicError &e) {
                std::cerr << e.what() << std::endl;
                return false;
            } catch (curlpp::RuntimeError &e) {
                std::cerr << e.what() << std::endl;
                return false;
            }

        return true;
    }
}
