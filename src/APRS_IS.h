/**
 * @file APRS_IS.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-08-30
 */

#pragma once

#include "basic_socket.h"
#include "APRS_Packet.h"

namespace aprs {
    using namespace std;
    using namespace sockets;

    class APRS_IS : public local_socket {
    public:
        std::string mPacket{};
        std::string mCallSign{};
        std::string mPassCode{};
        std::string mFilter{};
        std::string mPeerName{};
        std::string mServerVers{};
        std::size_t p0{0};
        std::size_t p1{std::string::npos};
        std::optional<double> mRadius{};

        APRS_Position mQthPosition{};

        bool mGoodServer = false;

        APRS_IS(const std::string &callsign, const std::string &passCode, const std::string &filter);

        std::string getPacket();

        char charAtIndex(int offset = 0) {
            return mPacket[p0 + offset];
        }

        char decodeCharAtIndex() {
            return mPacket[p0++];
        }

        char charAtEnd(int offset = 0) {
            return mPacket[p1 + offset];
        }

        std::size_t positionAt(const char c) {
            p0 = mPacket.find(c, p0);
            return p0;
        }

        std::size_t positionAfter(const char c) {
            p0 = positionAt(c) + 1;
            return p0;
        }

        auto doubleTerminateBy(const char c) {
            p1 = mPacket.find(c, p0);
            return std::strtod(mPacket.substr(p0, p1 - p0).c_str(), nullptr);
        }

        auto intTerminateBy(const char c) {
            p1 = mPacket.find(c, p0);
            return std::strtol(mPacket.substr(p0, p1 - p0).c_str(), nullptr, 10);
        }

        [[nodiscard]] std::string remainder() const {
            return mPacket.substr(p0);
        }

        std::string stringTerminateBy(const char c) {
            p1 = mPacket.find(c, p0);
            return mPacket.substr(p0, p1 - p0);
        }

        std::string decodeString(const std::size_t length) {
            auto str = mPacket.substr(p0, length);
            p0 += length;
            return str;
        }

        std::size_t skip() {
            p0 = p1 + 1;
            p1 = p0;
            return p0;
        }

        [[nodiscard]] bool prefix(const std::string &pre) const {
            return mPacket.rfind(pre, 0) == 0;
        }

        auto putLine(const std::string &line) {
            auto n = write(fd(), line.data(), line.length());
            if (n != line.length())
                cerr << "Only " << n << " of " << line.length() << " chars written.\n";
            return n;
        }

        bool openConnection();

        template<typename T>
        [[nodiscard]] std::optional<T> decodeValue(const std::size_t len, const T factor = 1.) {
            std::optional<T> value = safeConvert<T>(mPacket.substr(p0, len));
            if (value.has_value())
                value = value.value() * factor;
            p0 += len;
            return value;
        }

        enum class CoordinateType {
            LatitudeDDMMsss,
            LongitudeDDMMsss,
        };

        std::optional<double> decodeCoordinate(CoordinateType coordinateType);

        [[nodiscard]] std::unique_ptr<APRS_Packet> decode();
    };
}

