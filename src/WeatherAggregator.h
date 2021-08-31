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
        static constexpr std::size_t WeatherParam = 12;

        std::array<std::optional<double>, WeatherParam> mValueAggregate{};
        std::array<std::optional<double>, WeatherParam> mHannAggregate{};

        void clearAggregateData();

    public:

        void aggregateData();
    };
}

