//
// Created by richard on 2021-09-04.
//

#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>
#include <csignal>
#include <atomic>
#include "unixstd.h"
#include "InputParser.h"
#include "XDGFilePaths.h"
#include "ConfigFile.h"

using namespace std;
using namespace unixstd;

static std::atomic_bool run{true};

void signalHandler(int signum) {
    cerr << "Interrupt signal (" << signum << ") received.\n";

    // cleanup and close up stuff here
    // terminate program

    run = false;
}

int main(int argc, char **argv) {
    static constexpr std::string_view ConfigOption = "--config";

    enum class ConfigItem {
        InfluxTLS,
        InfluxHost,
        InfluxPort,
        InfluxDb,
        CPUZone,
    };

    std::vector<ConfigFile::Spec> ConfigSpec
            {{
                     {"influxTLS", ConfigItem::InfluxTLS},
                     {"influxHost", ConfigItem::InfluxHost},
                     {"influxPort", ConfigItem::InfluxPort},
                     {"influxDb", ConfigItem::InfluxDb},
                     {"cpuZone", ConfigItem::CPUZone}
             }};

    try {
        // Get the XDG environment and the configuration file path.
        xdg::Environment &environment{xdg::Environment::getEnvironment(true)};
        filesystem::path configFilePath = environment.appResourcesAppend("config.txt");

        // Ensure the system hostname is known.
        if (Hostname::name().empty() || Hostname::getError()) {
            cerr << "System hostname could not be determined: " << strerror(Hostname::getError()) << " Exiting.\n";
            exit(1);
        }

        // Parse command line arguments
        InputParser inputParser{argc, argv};

        // Allow override of the XDG located configuration file.
        if (inputParser.cmdOptionExists(ConfigOption))
            configFilePath = std::filesystem::path{inputParser.getCmdOption(ConfigOption)};

        // Set default values for influxDB host parameters.
        bool influxTls{false};
        std::optional<std::string> influxHost{};
        std::optional<long> influxPort{};
        std::optional<std::string> influxDb{};
        filesystem::path cpuZone{};

        // Catch signals
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        std::signal(SIGHUP, signalHandler);

        // Process the configuration file.
        ConfigFile configFile{configFilePath};
        if (auto status = configFile.open(); status == ConfigFile::OK) {
            bool validFile{true};
            configFile.process(ConfigSpec, [&](std::size_t idx, const std::string_view &data) {
                // ToDo: Implement item type decoding.
                bool validValue{false};
                switch (static_cast<ConfigItem>(idx)) {
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
                    case ConfigItem::CPUZone:
                        auto value = configFile.safeConvert<long>(data);
                        if (validValue = value.has_value(); validValue) {
                            stringstream strm{};
                            strm << "/sys/class/thermal/thermal_zone" << value.value() << "/temp";
                            cpuZone = strm.str();
                        }
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

            if (!filesystem::exists(cpuZone))
                cpuZone.clear();

            // Start the daemon process.
            while (run) {
                bool measurements = false;
                if (!cpuZone.empty()) {
                    std::ifstream ifs;
                    ifs.open(cpuZone, std::ofstream::in);
                    if (ifs) {
                        int temperature;
                        stringstream strm;
                        ifs >> temperature;
                        ifs.close();
                        strm << "cpuTemp=" << static_cast<double>(temperature) / 1000. << '\n';
                        cout << strm.str();
                        measurements = true;
                    }
                }

                if (measurements)
                    sleep(5);
                else {
                    cerr << "No active measurements, exiting\n";
                    run = false;
                }
            }
        } else if (status == ConfigFile::NO_FILE) {
            cerr << "Configuration file specified " << configFilePath << " does not exist.\n";
            exit(1);
        } else if (status == ConfigFile::OPEN_FAIL) {
            cerr << "Could not open configuration file " << configFilePath << ": "
                 << std::strerror(errno) << '\n';
            exit(1);
        }
    } catch (exception &e) {
        cerr << e.what() << '\n';
    }

    return 0;
}

