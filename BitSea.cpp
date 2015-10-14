#include "BitSea.hpp"

int main(int argc, char *argv[]) {
	cli::Settings programSettings;
	
	if(argc < 2) {
		std::cerr << "Not enough arguments." << std::endl;
		std::exit(1);
	}
	else {
		cli::parseCommandLine(argc, argv, programSettings);
	}
	
	TorrentFileParser torrentInfo(programSettings.fileName);	
	allocateStorage(torrentInfo);
	
	std::string announceUrl = torrentInfo.getAnnounce();
	std::string infoHash = torrentInfo.info.getHash();
	std::cout << "infoHash size: " << infoHash.length() << std::endl;
	Tracker tracker(announceUrl, infoHash);
	tracker.update();
}

void allocateStorage(TorrentFileParser &torrentInfo) {
	int mode = torrentInfo.info.getFileMode();
	std::vector<std::pair<std::string, int>> files;

	if(mode == InfoParser::FILEMODE_SINGLE) {
		std::string fileName = torrentInfo.info.getName();
		int length = torrentInfo.info.getLength();
		std::string md5 = torrentInfo.info.getMD5();
		files.push_back(std::make_pair(fileName, length));
	}
	else {
		for(boost::any file : torrentInfo.info.files) {
			std::unordered_map<std::string, boost::any> dict = boost::any_cast<std::unordered_map<std::string, boost::any>>(file);
			int length = InfoParser::fileLength(dict);
			std::string fileName = InfoParser::filePath(dict);
			std::string md5 = InfoParser::fileMD5(dict);
			files.push_back(std::make_pair(fileName, length));
		}
	}
	
	for(std::vector<std::pair<std::string, int>>::iterator it=files.begin(); it != files.end(); it++) {
		std::string fileName = it->first;
		int length = it->second;
		createFile(fileName, length);
	}
}

void createFile(std::string filePath, int size) {
	int pathLength = filePath.length();
	int position = 0;
	if(filePath[0] == '.' && filePath[1] == '/') {
		position = pathLength-1;
		while(filePath[position] != '/') {
			position--;
		}
		std::string directory = filePath.substr(0,position+1);
		boost::filesystem::path dir(directory);
		boost::filesystem::create_directories(dir);
		position++;
	}

	std::ofstream ofs(filePath, std::ios::binary | std::ios::out);
	ofs.seekp(size-1);
	ofs.write("",1);
	ofs.close();
}
