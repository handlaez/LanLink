#include "UnifiedArgParser.hpp"

#include <stdexcept>
#include <string_view>

Config ArgParser::parse(int argc, char* argv[])
{
    Config config;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "--nogui") {
            config.gui = false;
        }
        else if (arg == "-p" || arg == "--port") {
            if (++i >= argc) {
                throw std::runtime_error("--port requires a value");
            }

            try {
                config.port = std::stoi(argv[i]);
            }
            catch (const std::exception&) {
                throw std::runtime_error("Invalid port: " + std::string(argv[i]));
            }

            if (config.port < 1 || config.port > 65535) {
                throw std::runtime_error("Port must be between 1 and 65535");
            }
        }
        else if (arg == "--ip") {
            if (++i >= argc) {
                throw std::runtime_error(std::string(arg) + " requires a value");
            }

            config.ip = argv[i];
        }
        else if (arg == "-prod" || arg == "--producer") {
            config.producer = true;
        }
        else {
            throw std::runtime_error("Unknown argument: " + std::string(arg));
        }
    }

    return config;
}