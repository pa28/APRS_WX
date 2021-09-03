/**
 * @file WeatherAggregator.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

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
            if (diff.count() > 3600.)
                it = erase(it);
            else
                ++it;
        }
    }

    std::ostream &WeatherAggregator::printInfluxFormat(ostream &strm, const std::string &prefix) const {
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
                    strm << prefix << item.dbName << '=' << value << '\n';
                }
            }
        }

        if (temperature.has_value() && relHumidity.has_value()) {
            auto celsius = FahrenheitToCelsius(temperature.value());
            auto dewPoint = celsius - ((100. - relHumidity.value()) / 5.);
            auto e = 6.11 * exp(5417.7530 * ((1. / 273.16) - (1. / (dewPoint + 273.15))));
            auto h = 0.5555 * (e - 10.0);
            auto humidex = celsius + h;
            strm << prefix << "DewPt=" << dewPoint << '\n';
            strm << prefix << "Humidex=" << humidex << '\n';
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
//
//        cout << "\tAgregate from " << size() << " stations.\n";
//        for (const auto &item : WeatherItemList) {
//            auto idx = static_cast<std::size_t>(item.wxSym);
//            if (item.wxFlag != 'l' && mValueAggregate[idx].has_value())
//                cout << '\t' << item.dbName << ": " << mValueAggregate[idx].value() / mHannAggregate[idx].value() << '\n';
//        }
//
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
                std::cout << e.what() << std::endl;
                return false;
            } catch (curlpp::RuntimeError &e) {
                std::cout << e.what() << std::endl;
                return false;
            }

        return true;
    }
}
