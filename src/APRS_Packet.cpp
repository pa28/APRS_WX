/**
 * @file APRS_Packet.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#include "APRS_Packet.h"

namespace aprs {
    using namespace std;

    std::ostream &APRS_Packet::printOn(std::ostream &strm) const {
        strm << mName << ' ' << mSymTableId << mSymCode;
        return strm;
    }

    bool APRS_Position::setBearingDistance(const APRS_Position &other) {
        if (mLon.has_value() && mLon.has_value() && other.mLat.has_value() && other.mLon.has_value()) {
            auto lat2 = deg2rad(mLat.value());
            auto lon2 = deg2rad(mLon.value());
            auto lat1 = deg2rad(other.mLat.value());
            auto lon1 = deg2rad(other.mLon.value());

            auto sinLat = sin((lat1 - lat2) / 2);
            auto sinLatSq = sinLat * sinLat;
            auto sinLon = sin((lon1 - lon2) / 2.);
            auto sinLonSq = sinLon * sinLon;
            auto d1 = 2. * asin(sqrt(sinLatSq + cos(lat1) * cos(lat2) * sinLonSq));
            auto d0 = acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon1 - lon2));
            mDistance = d1 * 6371.;

            double dlon = lon2 - lon1;
            double tc = atan2(sin(dlon) * cos(lat2), cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dlon));
            if (tc < 0.)
                tc = 2. * M_PI + tc;
            mBearing = rad2deg(tc);

            return true;
        }
        return false;
    }

    std::ostream &APRS_Position::printOn(std::ostream &strm) const {
        APRS_Packet::printOn(strm);
        if (mLat.has_value() && mLon.has_value())
            strm << ' ' << fixed << setprecision(4) << mLat.value() << ',' << mLon.value();
        if (mDistance.has_value() && mBearing.has_value())
            strm << ' ' << setprecision(1) << mDistance.value() << " @ " << setprecision(0) << mBearing.value()
                 << " deg";
        if (mHannValue.has_value())
            strm << " Hann value: " << setprecision(3) << mHannValue.value();

        strm << setprecision(0) << defaultfloat;
        return strm;
    }

    std::ostream &APRS_WX_Report::printOn(ostream &strm) const {
        APRS_Position::printOn(strm);
        strm << "\n\t" << fixed;
        if (mWindDir.has_value() && mWindSpeed.has_value())
            strm << setprecision(1) << mWindSpeed.value() << " m/s @ " << setprecision(0) << mWindDir.value()
                 << " deg ";
        if (mWindGust.has_value())
            strm << "Gust " << setprecision(1) << mWindGust.value() << " m/s ";
        if (mTemperature.has_value())
            strm << "Temp " << mTemperature.value() << "C ";
        if (mHumidity.has_value())
            strm << "Humidity " << mHumidity.value() << "% ";
        if (mRainDay.has_value() || mRainHour.has_value() || mRainMidnight.has_value())
            strm << "Rain ";
        if (mRainHour.has_value())
            strm << mRainHour.value() << " mm/hr ";
        if (mRainDay.has_value())
            strm << mRainDay.value() << " mm/day ";
        if (mRainMidnight.has_value())
            strm << mRainMidnight.value() << " mm since midnight ";
        if (mSnow.has_value())
            strm << "Snow " << mSnow.has_value() << " cm. ";
        if (mBarometricPressure.has_value())
            strm << "Bp " << mBarometricPressure.value() << " hPa ";
        if (mLuminosity.has_value())
            strm << "Lumi " << mLuminosity.value() << " W/sqm ";
        strm << setprecision(0) << defaultfloat;
        return strm;
    }
}

