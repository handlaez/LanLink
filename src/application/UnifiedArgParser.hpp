#ifndef UNIFIED_ARG_PARSER_HPP
#define UNIFIED_ARG_PARSER_HPP

#include <string>
#include <vector>
#include <stdexcept>

struct Config {
	bool verbose = false;
	bool gui = true;
	int port = 5000;
	std::string ip = "192.168.0.100";
	bool producer = false;
};

class ArgParser {
public:
	static Config parse(int argc, char* argv[]);

};

#endif