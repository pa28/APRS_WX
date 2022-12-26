/**
 * @file ConfigFile.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-09-02
 */

#include <iostream>
#include <functional>
#include <optional>
#include "ConfigFile.h"

ConfigFile::Status ConfigFile::open() {
    if (std::filesystem::exists(mConfigFilePath)) {
        mIstrm.open(mConfigFilePath);
        if (mIstrm) {
            return Status::OK;
        }
        return Status::OPEN_FAIL;
    }
    return Status::NO_FILE;
}

void ConfigFile::close() {
    mIstrm.close();
}

ConfigFile::Status ConfigFile::process(const std::vector<ConfigFile::Spec>& configSpecs, const std::function<void(std::size_t, const std::string_view&)>& callback) {
    std::string line{};

    while (std::getline(mIstrm, line)) {
        if (line[0] != '#') {
            for (auto &spec : configSpecs) {
                if (auto n = matchKey(line.begin(), line.end(), spec.mKey); n != std::string::npos) {
                    callback(spec.mIdx,std::string_view{line.substr(n)});
                    break;
                }
            }
        }
    }

    return ConfigFile::OK;
}

std::string::size_type
ConfigFile::matchKey(const std::string::iterator first, const std::string::iterator last, const std::string_view &key) {
    auto idx = first;
    for (auto c : key) {
        if (c != *idx)
            return std::string::npos;
        ++idx;
    }

    while (idx != last && isspace(*idx))
        ++idx;

    return idx - first;
}
