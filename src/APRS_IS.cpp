/**
 * @file APRS_IS.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#include <algorithm>
#include "APRS_IS.h"

using namespace std;

namespace aprs {

    APRS_IS::APRS_IS(const std::string &callsign, const std::string &passCode, const std::string &filter) :
            local_socket("cwop.aprs2.net", "14580", true) {
        mCallSign = callsign;
        mPassCode = passCode;
        mFilter = filter;
    }

    std::string APRS_IS::getPacket() {
        std::stringstream stringBuf;
        char buf[2];
        bool endOfLine = false;
        while (!endOfLine) {
            if (auto n = select(std::chrono::seconds(1)); n > 0) {
                switch (read(fd(), buf, 1)) {
                    case -1:
                        exit(1);
                    default:
                        if (buf[0] != '\r')
                            stringBuf << buf[0];
                        endOfLine = buf[0] == '\n';
                }
            } else if (n != 0) {
                cerr << "Select: " << n << '\n';
                exit(1);
            }
        }

        mPacket = stringBuf.str();
        p0 = 0;
        p1 = std::string::npos;
        return mPacket;
    }

    bool APRS_IS::openConnection() {
        while (!mGoodServer) {
            if (connect(AF_INET6, AF_INET, AF_UNSPEC) < 0) {
                cerr << "Connection failed.\n";
                return false;
            }

            mPeerName = getPeerName();
            getPacket();
            if (mGoodServer = !prefix("# javAPRSSrvr 4.3.0b22") &&
                              !prefix("# javAPRSSrvr 4.3.0b17") &&
                              !prefix("# javAPRSSrvr 4.2.0b09"); !mGoodServer) {
                cerr << "Reject " << mPeerName << " version " << mPacket;
                close();
            }
        }

        cerr << "Accept " << mPeerName << " version " << mPacket;

        std::stringstream validate;
        validate << "user " << mCallSign << " pass " << mPassCode;
        if (!mFilter.empty())
            validate << " filter " << mFilter;
        validate << "\r\n";

        cout << validate.str();
        putLine(validate.str());
        getPacket();
        cout << mPacket;
        return true;
    }

    std::optional<double> APRS_IS::decodeCoordinate(APRS_IS::CoordinateType coordinateType) {
        char positiveChar, negativeChar;
        std::size_t length, minuteLength, degreeLength;

        switch (coordinateType) {
            case CoordinateType::LatitudeDDMMsss:
                positiveChar = 'N';
                negativeChar = 'S';
                degreeLength = 2;
                minuteLength = 5;
                break;
            case CoordinateType::LongitudeDDMMsss:
                positiveChar = 'E';
                negativeChar = 'W';
                degreeLength = 3;
                minuteLength = 5;
                break;
        }

        length = degreeLength + minuteLength;

        std::optional<long> degrees = decodeValue<long>(degreeLength);
        std::optional<double> minutes = decodeValue<double>(minuteLength);
        char c = static_cast<char>(toupper(decodeCharAtIndex()));
        if (degrees.has_value() && minutes.has_value()) {
            minutes.value() = minutes.value() / 60. + static_cast<double>(degrees.value());
            if (c == positiveChar) {
                return minutes;
            } else if (c == negativeChar) {
                minutes.value() *= -1;
                return minutes;
            }
        }
        return std::nullopt;
    }

    std::unique_ptr<APRS_Packet> APRS_IS::decode() {
        auto name = stringTerminateBy('>');
        positionAfter(':');
        while (p0 < mPacket.length() && p0 != std::string::npos) {
            auto descriminator = decodeCharAtIndex();
            switch (descriminator) {
                case '!':
                case '=':
                case '@':
                case '/': {
                    auto wxReport = std::make_unique<APRS_WX_Report>();
                    wxReport->mName = name;

                    if (descriminator == '@' || descriminator == '/')
                        wxReport->mDateTime = decodeString(7);

                    if (auto res = decodeCoordinate(CoordinateType::LatitudeDDMMsss); res.has_value()) {
                        wxReport->mLat = res.value();
                    } else {
                        wxReport->mPacketStatus = PacketStatus::ErrorLatitude;
                        return wxReport;
                    }

                    wxReport->mSymTableId = decodeCharAtIndex();

                    if (auto res = decodeCoordinate(CoordinateType::LongitudeDDMMsss); res.has_value()) {
                        wxReport->mLon = res.value();
                    } else {
                        wxReport->mPacketStatus = PacketStatus::ErrorLongitude;
                        return wxReport;
                    }

                    wxReport->mSymCode = decodeCharAtIndex();

                    wxReport->decodeWeatherValue(*this, WxSym::WindDirection);
                    ++p0;
                    wxReport->decodeWeatherValue(*this, WxSym::WindSpeed);

                    bool inWx = true;
                    while (inWx && p0 < mPacket.length()) {
#if 0
                        switch (decodeCharAtIndex()) {
                            case 'c':
                                wxReport->mWindDir = decodeValue<long>(3);
                                break;
                            case 'g':
                                wxReport->mWindGust = decodeValue<double>(3, 0.44704);
                                break;
                            case 't':
                                wxReport->mTemperature = decodeValue<double>(3);
                                if (wxReport->mTemperature.has_value())
                                    wxReport->mTemperature = (wxReport->mTemperature.value() - 32.) * 0.555555556;
                                break;
                            case 'r':
                                wxReport->mRainHour = decodeValue<double>(3, 0.254);
                                break;
                            case 'p':
                                wxReport->mRainDay = decodeValue<double>(3, 0.254);
                                break;
                            case 'P':
                                wxReport->mRainMidnight = decodeValue<double>(3, 0.254);
                                break;
                            case 'h':
                                wxReport->mHumidity = decodeValue<long>(2);
                                break;
                            case 'b':
                                wxReport->mBarometricPressure = decodeValue<double>(5, 0.1);
                                break;
                            case 'L':
                                wxReport->mLuminosity = decodeValue<long>(3);
                                break;
                            case 'l':
                                wxReport->mLuminosity = decodeValue<long>(3);
                                if (wxReport->mLuminosity.has_value())
                                    wxReport->mLuminosity = wxReport->mLuminosity.value() + 1000;
                                break;
                            case 's':
                                wxReport->mSnow = decodeValue<long>(3, 2.54);
                                break;
                            default:
                                inWx = false;
                        }
#else
                        auto flag = decodeCharAtIndex();
                        try {
                            if (auto found = find_if(WeatherItemList.begin(), WeatherItemList.end(), [flag](auto item){
                                    return item.wxFlag == flag;
                            } ); found != WeatherItemList.end()) {
                                wxReport->decodeWeatherValue(*this, found->wxSym, flag, found->factor);
                            } else {
                                inWx = false;
                            }
                        } catch (const WeatherValueError &weatherValueError) {
                            cerr << "Weather value decoding error: " << weatherValueError.what()
                                 << " Index: " << p0 << '\n'
                                 << '\t' << mPacket << '\n';
                            inWx = false;
                        }
#endif
                    }
                    --p0;

                    wxReport->setBearingDistance(mQthPosition);
                    if (mRadius.has_value() && wxReport->mDistance.has_value()) {
                        double hann = sin(
                                (M_PI * (mRadius.value() - wxReport->mDistance.value())) / (mRadius.value() * 2.));
                        wxReport->mHannValue = hann * hann;
                    }

                    wxReport->mPacketStatus = PacketStatus::WxPacket;
                    return wxReport;
                }
                    break;
                default:
                    cerr << mPacket << "Unhandled discriminator " << '"' << descriminator << '"' << " at " << p0
                         << '\n';
                    return std::make_unique<APRS_Packet>(PacketStatus::DecodingError);
            }
        }

        return std::make_unique<APRS_Packet>();
    }

}
