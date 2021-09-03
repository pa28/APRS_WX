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
    };

    std::vector<ConfigFile::Spec> ConfigSpec
            {{
                     {"callsign", static_cast<std::size_t>(ConfigItem::Callsign)},
                     {"passcode", static_cast<std::size_t>(ConfigItem::Passcode)},
                     {"latitude", static_cast<std::size_t>(ConfigItem::Latitude)},
                     {"longitude", static_cast<std::size_t>(ConfigItem::Longitude)},
                     {"radius", static_cast<std::size_t>(ConfigItem::Radius)},
             }};

    xdg::Environment &environment{xdg::Environment::getEnvironment()};
    filesystem::path configFilePath = environment.appResourcesAppend("config.txt");

    InputParser inputParser{argc, argv};
    std::string callsign{};
    std::string passCode{};
    std::string filter{};
    std::optional<double> qthLatitude{};
    std::optional<double> qthLongitude{};
    std::optional<long> filterRadius{};

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGHUP, signalHandler);

    WeatherAggregator weatherAggregator{};

    if (inputParser.cmdOptionExists(ConfigOption))
        configFilePath = std::filesystem::path{inputParser.getCmdOption(ConfigOption)};

    ConfigFile configFile{configFilePath};
    if (auto status = configFile.open(); status == ConfigFile::OK) {
        configFile.process(ConfigSpec, [](std::size_t idx, const std::string_view& data){
            // ToDo: Implement item type decoding.
            switch (static_cast<ConfigItem>(idx)) {
                case ConfigItem::Callsign:
                case ConfigItem::Passcode:
                case ConfigItem::Latitude:
                case ConfigItem::Longitude:
                case ConfigItem::Radius:
                    cout << idx << ": '" << data << "'\n";
            }
        });
        configFile.close();
    } else if (status == ConfigFile::NO_FILE) {
        cerr << "Configuation file specified " << configFilePath << " does not exist.\n";
        exit(1);
    } else if (status == ConfigFile::OPEN_FAIL) {
        cerr << "Could not open configuration file " << configFilePath << ": " << std::strerror(errno) << '\n';
        exit(1);
    }

    std::cout << "Hello, CWOP APRS-IS!" << '\n'
              << callsign << ' ' << passCode << ' ' << filter << '\n';

    APRS_IS sock{callsign, passCode, filter};
    sock.mQthPosition.mLat = qthLatitude;
    sock.mQthPosition.mLon = qthLongitude;
    sock.mRadius = filterRadius;

    if (sock.openConnection()) {
        while (run) {
            sock.getPacket();
            if (!sock.mPacket.empty()) {
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
                                weatherAggregator.pushToInflux("pidp11.local", 8086, "aprs_wx");
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

