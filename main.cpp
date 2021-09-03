//
// Created by richard on 2021-08-30.
//

#include <iostream>
#include <iomanip>
#include <optional>
#include <vector>
#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstring>
#include "src/XDGFilePaths.h"
#include "src/ConfigFile.h"
#include "src/APRS_Packet.h"
#include "src/APRS_IS.h"
#include "src/WeatherAggregator.h"

using namespace std;
using namespace sockets;

/**
 * @class InputParser
 * @brief Parse command line arguments.
 */
class InputParser {
public:
    /**
     * @brief Constructor
     * @param argc The number of command line arguments passed to the application
     * @param argv The array of command line arguments.
     */
    InputParser(int &argc, char **argv) {
        for (int i = 1; i < argc; ++i)
            this->tokens.emplace_back(argv[i]);
    }

    /// @author iain
    [[nodiscard]] const std::string &getCmdOption(const std::string_view &option) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end()) {
            return *itr;
        }
        static const std::string empty_string;
        return empty_string;
    }

    /// @author iain
    [[nodiscard]] bool cmdOptionExists(const std::string_view &option) const {
        return std::find(this->tokens.begin(), this->tokens.end(), option)
               != this->tokens.end();
    }

private:
    std::vector<std::string> tokens;
};

static std::atomic_bool run{true};

void usage(const std::string &app) {
    cout << "Usage: " << app
         << " --call <callsign> --pass <passcode> --lat <decimal degrees> -lon <decimal degrees --radius <km>\n";
    exit(0);
}

using namespace std;
using namespace aprs;

void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received.\n";

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
                     {"influxDb", ConfigItem::InfluxDb}
             }};

    xdg::Environment &environment{xdg::Environment::getEnvironment()};
    filesystem::path configFilePath = environment.appResourcesAppend("config.txt");

    InputParser inputParser{argc, argv};
    std::optional<std::string> callsign{};
    std::optional<std::string> passCode{};
    std::string filter{};
    std::optional<double> qthLatitude{};
    std::optional<double> qthLongitude{};
    std::optional<long> filterRadius{};

    bool influxTls{false};
    std::optional<std::string> influxHost{};
    std::optional<long> influxPort{};
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
        configFile.process(ConfigSpec, [&](std::size_t idx, const std::string_view &data)
        {
            // ToDo: Implement item type decoding.
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
                case ConfigItem::InfluxTLS: {
                    std::optional<long> value = configFile.safeConvert<long>(data);
                    validValue = value.has_value();
                    if (validValue)
                        influxTls = value.value() != 0;
                }
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
        cerr << "Could not open configuration file " << configFilePath << ": " <<

             std::strerror(errno)

             << '\n';
        exit(1);
    }

    std::stringstream filterStrm{};
    filterStrm << "r/" << qthLatitude.

            value()

               << '/' << qthLongitude.

            value()

               << '/' << filterRadius.

            value();

    filter = filterStrm.str();

    std::cout << "Hello, CWOP APRS-IS!" << '\n'
              << callsign.

                      value()

              << ' ' << passCode.

            value()

              << ' ' << filter << '\n';

    APRS_IS sock{callsign.value(), passCode.value(), filter};
    sock.mQthPosition.
            mLat = qthLatitude;
    sock.mQthPosition.
            mLon = qthLongitude;
    sock.
            mRadius = filterRadius;

    if (sock.

            openConnection()

            ) {
        while (run) {
            sock.

                    getPacket();

            if (!sock.mPacket.

                    empty()

                    ) {
                if (!sock.prefix("# aprsc")) {
                    if (sock.charAtIndex() != '#') {
                        auto packet = sock.decode();
                        switch (packet->status()) {
                            case PacketStatus::WxPacket: {
                                packet->printOn(cout) << '\n';
                                auto wx = std::unique_ptr<APRS_WX_Report>(
                                        dynamic_cast<APRS_WX_Report *>(packet.release()));
                                weatherAggregator[wx->mName] = std::move(wx);
                                weatherAggregator.aggregateData();
                                if (influxHost.has_value() && influxPort.has_value() && influxDb.has_value())
                                    weatherAggregator.pushToInflux(influxHost.value(), influxTls, influxPort.value(), influxDb.value());
                            }
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    sock.close();

    return 0;
}

