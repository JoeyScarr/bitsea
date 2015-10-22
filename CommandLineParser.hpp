// CommandLineParser.hpp
#ifndef COMMAND_LINE_PARSER_HPP
#define COMMAND_LINE_PARSER_HPP

#include <string>
#include <stdint.h>
#include <vector>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace cli {
	const int COMMAND_MAX = 256;
	const int PEER_THRESHHOLD = 10;
	const int MAX_DOWNLOADS = 15;
	const int MAX_UPLOADS = 10;
	
	struct Settings {
		std::string fileName;
		std::uint16_t port;
		std::uint16_t peerThreshold;
		std::uint16_t maxDownloads;
		std::uint16_t maxUploads;
		
		Settings() {
			peerThreshold = PEER_THRESHHOLD;
			maxDownloads = MAX_DOWNLOADS;
			maxUploads = MAX_UPLOADS;
		}
	};

	void parseCommandLine(int argc, char *argv[], Settings &programSettings);
	void parseCommand(const std::string &command, std::vector<std::string> &arguments, cli::Settings &programSettings);
}

#endif
