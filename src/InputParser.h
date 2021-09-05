/**
 * @file InputParser.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2021-09-05
 */

#pragma once

#include <vector>
#include <string>
#include <algorithm>

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
