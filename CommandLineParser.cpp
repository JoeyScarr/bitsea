// CommandLineParser.cpp
#include "CommandLineParser.hpp"

void cli::parseCommandLine(const int argc, char *argv[], Settings &programSettings) {
	
	if(argc < 2) {
		std::cerr << "Not enough arguments." << std::endl;
		std::exit(1);
	}
	
	int commandList[argc];
	int commandCount = 0;
	
	for(int i=1; i < argc; i++) {
		if(argv[i][0] == '-') {
			commandList[commandCount] = i;
			commandCount++;
		}
	}
		
	for(int i=0; i < commandCount; i++) {
		std::vector<std::string> arguments;

		std::string command = &(argv[commandList[i]][1]);
		int argEnd;
		if(i < commandCount-1)
			argEnd = commandList[i+1];
		else
			argEnd = argc-1;
			
		for(int j=commandList[i]+1; j < argEnd; j++)
			arguments.push_back(argv[j]);
			
		parseCommand(command, arguments, programSettings);
	}
		
	programSettings.fileName = argv[argc-1];
}

void cli::parseCommand(const std::string &command, std::vector<std::string> &arguments, Settings &programSettings) {
	if(command == "p") {
		programSettings.port = std::stoi(arguments.front());
		std::cout << "port is " << programSettings.port << std::endl;
	}
}
