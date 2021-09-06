//
// Created by richard on 2021-09-04.
//

#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
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

            // Build the influx server URL
            stringstream url{};
            url << (influxTls? "https" : "http") << "://" << influxHost.value() << ':'
                << influxPort.value() << "/write?db=" << influxDb.value();

            // Build the influx data prefix
            stringstream prefix{};
            prefix << "sys,host=" << Hostname::name() << ' ';

            // Start the daemon process.
            while (run) {
                stringstream measurements{};
                if (!cpuZone.empty()) {
                    std::ifstream ifs;
                    ifs.open(cpuZone, std::ofstream::in);
                    if (ifs) {
                        int temperature;
                        ifs >> temperature;
                        ifs.close();
                        measurements << prefix.str() << "cpuTemp=" << temperature / 1000 << '\n';
                    }
                }

                auto data = measurements.str();
                if (!data.empty()) {
                    try {
                        cURLpp::Cleanup cleaner;
                        cURLpp::Easy request;
                        request.setOpt(new cURLpp::Options::Url(url.str()));
                        request.setOpt(new curlpp::options::Verbose(false));

                        std::list<std::string> header;
                        header.emplace_back("Content-Type: application/octet-stream");
                        request.setOpt(new curlpp::options::HttpHeader(header));

                        request.setOpt(new curlpp::options::PostFieldSize(data.length()));
                        request.setOpt(new curlpp::options::PostFields(data));

                        request.perform();
                    } catch (curlpp::LogicError &e) {
                        std::cerr << e.what() << std::endl;
                        return 1;
                    } catch (curlpp::RuntimeError &e) {
                        std::cerr << e.what() << std::endl;
                        return 1;
                    }
                    sleep(30);
                } else {
                    cerr << "No active measurements, exiting\n";
                    run = false;
                }
            }
        } else if (status == ConfigFile::NO_FILE) {
            cerr << "Configuration file specified " << configFilePath << " does not exist.\n";
            return 1;
        } else if (status == ConfigFile::OPEN_FAIL) {
            cerr << "Could not open configuration file " << configFilePath << ": "
                 << std::strerror(errno) << '\n';
            return 1;
        }
    } catch (exception &e) {
        cerr << e.what() << '\n';
    }

    return 0;
}

