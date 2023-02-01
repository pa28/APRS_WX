//
// Created by richard on 2021-08-30.
//

#include <iostream>
#include <iomanip>
#include <optional>
#include <vector>
#include <cmath>
#include <csignal>
#include <cstring>
#include "InputParser.h"
#include "XDGFilePaths.h"
#include "ConfigFile.h"
#include "APRS_Packet.h"
#include "APRS_IS.h"
#include "WeatherAggregator.h"

using namespace std;
using namespace sockets;

static std::atomic_bool run{true};

[[maybe_unused]] void usage(const std::string &app) {
    cout << "Usage: " << app
         << " --call <callsign> --pass <passcode> --lat <decimal degrees> -lon <decimal degrees --radius <km>\n";
    exit(0);
}

using namespace std;
using namespace aprs;

void signalHandler(int signum) {
    cerr << "Interrupt signal (" << signum << ") received.\n";

    // cleanup and close up stuff here
    // terminate program

    run = false;
}

int main(int argc, char **argv) {
    static constexpr std::string_view ConfigOption = "--config";

    enum class ConfigItem {
        Callsign,
        Passcode,
        Latitude,
        Longitude,
        Radius,
        InfluxTLS,
        InfluxHost,
        InfluxPort,
        InfluxDb,
        InfluxRepeats,
        ServerCycleRate,
    };

    std::vector<ConfigFile::Spec> ConfigSpec
            {{
                     {"callsign", ConfigItem::Callsign},
                     {"passcode", ConfigItem::Passcode},
                     {"latitude", ConfigItem::Latitude},
                     {"longitude", ConfigItem::Longitude},
                     {"radius", ConfigItem::Radius},
                     {"influxTLS", ConfigItem::InfluxTLS},
                     {"influxHost", ConfigItem::InfluxHost},
                     {"influxPort", ConfigItem::InfluxPort},
                     {"influxDb", ConfigItem::InfluxDb},
                     {"influxRepeats", ConfigItem::InfluxRepeats},
                     {"cycleRate", ConfigItem::ServerCycleRate},
             }};

    try {
        xdg::Environment &environment{xdg::Environment::getEnvironment(true)};
        filesystem::path configFilePath = environment.appResourcesAppend("config.txt");

        InputParser inputParser{argc, argv};
        std::optional<std::string> callsign{};
        std::optional<std::string> passCode{};
        std::string filter{};
        std::optional<double> qthLatitude{};
        std::optional<double> qthLongitude{};
        std::optional<long> filterRadius{};

        std::optional<bool> influxTls{false};
        std::optional<bool> influxRepeats{false};
        std::optional<unsigned long> serverCycleRate{100};
        std::optional<std::string> influxHost{};
        std::optional<unsigned> influxPort{};
        std::optional<std::string> influxDb{};

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        std::signal(SIGHUP, signalHandler);

        WeatherAggregator weatherAggregator{};

        if (inputParser.cmdOptionExists(ConfigOption))
            configFilePath = std::filesystem::path{inputParser.getCmdOption(ConfigOption)};

        ConfigFile configFile{configFilePath};
        if (auto status = configFile.open(); status == ConfigFile::OK) {
            bool validFile{true};
            configFile.process(ConfigSpec, [&](std::size_t idx, const std::string_view &data) {
                bool validValue{false};
                switch (static_cast<ConfigItem>(idx)) {
                    case ConfigItem::Callsign:
                        callsign = ConfigFile::parseText(data, [](char c) {
                            return ConfigFile::isalnum(c) || c == '-';
                        }, ConfigFile::toupper);
                        validValue = callsign.has_value();
                        break;
                    case ConfigItem::Passcode:
                        passCode = ConfigFile::parseText(data, ConfigFile::isdigit);
                        validValue = passCode.has_value();
                        break;
                    case ConfigItem::Latitude:
                        qthLatitude = configFile.safeConvert<double>(data);
                        validValue = qthLatitude.has_value();
                        break;
                    case ConfigItem::Longitude:
                        qthLongitude = configFile.safeConvert<double>(data);
                        validValue = qthLongitude.has_value();
                        break;
                    case ConfigItem::Radius:
                        filterRadius = configFile.safeConvert<long>(data);
                        validValue = filterRadius.has_value();
                        break;
                    case ConfigItem::InfluxTLS:
                        influxTls = ConfigFile::parseBoolean(data);
                        validValue = influxTls.has_value();
                        break;
                    case ConfigItem::InfluxHost:
                        influxHost = ConfigFile::parseText(data, [](char c) {
                            return ConfigFile::isalnum(c) || c == '.';
                        });
                        validValue = influxHost.has_value();
                        break;
                    case ConfigItem::InfluxPort:
                        influxPort = configFile.safeConvert<long>(data);
                        validValue = influxPort.has_value() && influxPort.value() > 0;
                        break;
                    case ConfigItem::InfluxDb:
                        influxDb = ConfigFile::parseText(data, [](char c) {
                            return ConfigFile::isalnum(c) || c == '_';
                        });
                        validValue = influxDb.has_value();
                        break;
                    case ConfigItem::InfluxRepeats:
                        influxRepeats = ConfigFile::parseBoolean(data);
                        validValue = influxRepeats.has_value();
                        break;
                    case ConfigItem::ServerCycleRate:
                        serverCycleRate = configFile.safeConvert<unsigned long>(data);
                        validValue = serverCycleRate.has_value();
                        break;
                }
                validFile = validFile & validValue;
                if (!validValue) {
                    std::cerr << "Invalid config value: " << ConfigSpec[idx].mKey << '\n';
                }
            });
            configFile.close();
            if (!validFile) {
                std::cerr << "Invalid configuration file " << configFilePath << " exiting.\n";
            }

            if (!(influxHost.has_value() && influxPort.has_value() && influxDb.has_value()))
                std::cerr << "Influx database not specified in " << configFilePath << ", data will not be stored.\n";
        } else if (status == ConfigFile::NO_FILE) {
            cerr << "Configuration file specified " << configFilePath << " does not exist.\n";
            exit(1);
        } else if (status == ConfigFile::OPEN_FAIL) {
            cerr << "Could not open configuration file " << configFilePath << ": "
                 << std::strerror(errno) << '\n';
            exit(1);
        }

        if (callsign.has_value() && callsign.value().rfind("N0CALL") == 0) {
            cerr << "Configuration file " << configFilePath << " not configured.\n";
            exit(1);
        }

        std::stringstream filterStrm{};
        filterStrm << "r/" << qthLatitude.value()
                   << '/' << qthLongitude.value()
                   << '/' << filterRadius.value();

        filter = filterStrm.str();

        std::cerr << "Hello, CWOP APRS-IS!" << '\n'
                  << callsign.value()
                  << ' ' << filter << '\n';

        while (run) {
            APRS_IS sock{callsign.value(), passCode.value(), filter};
            sock.mQthPosition.mLat = qthLatitude;
            sock.mQthPosition.mLon = qthLongitude;
            sock.mRadius = filterRadius;

            unsigned long packetCount = 0;

            if (sock.openConnection()) {
                while (packetCount < serverCycleRate.value()) {
                    sock.getPacket();

                    if (!sock.mPacket.empty()) {
                        std::cerr << sock.mPacket;
                        ++packetCount;
                        if (!sock.prefix("# aprsc")) {
                            if (sock.charAtIndex() != '#') {
                                auto packet = sock.decode();
                                switch (packet->status()) {
                                    case PacketStatus::WxPacket: {
                                        auto wx = std::unique_ptr<APRS_WX_Report>(
                                                dynamic_cast<APRS_WX_Report *>(packet.release()));
                                        weatherAggregator[wx->mName] = std::move(wx);
                                        weatherAggregator.aggregateData();
                                        if (influxHost.has_value() && influxPort.has_value() && influxDb.has_value())
                                            weatherAggregator.pushToInflux(influxHost.value(), influxTls.value(),
                                                                           influxPort.value(), influxDb.value());
                                    }
                                        break;
                                    case PacketStatus::DecodingError:
                                        cerr << "Packet decoding error.\n";
                                        return 1;
                                    default:
                                        break;
                                }
                            }
                        } else {
                            if (influxRepeats.value() && !weatherAggregator.empty() && influxHost.has_value() &&
                                influxPort.has_value() && influxDb.has_value())
                                weatherAggregator.pushToInflux(influxHost.value(), influxTls.value(),
                                                               influxPort.value(), influxDb.value());
                        }
                    } else {
                        std::cerr << "*** Empty packet.\n";
                        packetCount = serverCycleRate.value();
                    }
                }
            }

            sock.close();
        }
    } catch (exception &e) {
        cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}

