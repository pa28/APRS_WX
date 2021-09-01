/**
 * @file WeatherAggregator.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#pragma once

#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <ios>

#include "APRS_Packet.h"

namespace aprs {
    using namespace std;

    class WeatherAggregator : public std::map<std::string, std::unique_ptr<APRS_WX_Report>> {
    protected:

        std::array<std::optional<double>, WeatherItemCount> mValueAggregate{};
        std::array<std::optional<double>, WeatherItemCount> mHannAggregate{};

        void clearAggregateData();

        std::ostream &printInfluxFormat(ostream &strm, const std::string &prefix) const;

    public:
        bool pushToInflux(const string &host, unsigned int port, const std::string& dataBase);

        static double FahrenheitToCelsius(double fahrenheit) {
            return (fahrenheit - 32.) * (5./9.);
        }

        void aggregateData();
    };
}

