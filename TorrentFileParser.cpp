#include "TorrentFileParser.hpp"

TorrentFileParser::TorrentFileParser(std::string torrentFileName) {
	std::ifstream torrentFile(torrentFileName);
	torrentFile.seekg(0, std::ios::end);
	size_t size = torrentFile.tellg();
	std::string buffer(size, ' ');
	torrentFile.seekg(0);
	torrentFile.read(&buffer[0], size); 

	BDecoder torrentFileDecoder(buffer, 0);
	bool status = torrentFileDecoder.decode();
	
	if(!status) {
		std::cerr << "Error decoding file.\n";
		std:exit(1);
	}
	
	dictionary = torrentFileDecoder.get();
	
	if(dictionary.empty()) {
		std::cerr << "Error fetching dictionary.\n";
		std::exit(1);
	}
	
	info.rawString = torrentFileDecoder.getRawInfoDict();
	process();
}

void TorrentFileParser::process() {
	try {
		creationDate = getCreationDate();
	}
	catch(int e) {}
	
	try {
		announce = getAnnounce();
	}
	catch(int e) {
		std::cerr << "Missing announce.\n";
		exit(1);
	}
	
	try {
		comment = getComment();
	} catch(int e) {}
	
	try {
		createdBy = getCreatedBy();
	} catch(int e) {}
	
	try {
		encoding = getEncoding();
	} catch(int e) {}
	
	try {
		announceList = getAnnounceList();
	} catch(int e) {}
	
	try {
		info.initialise(getInfo());
	}
	catch(int e) {
		std::cerr << "Missing info dictionary.\n";
		exit(1);
	}
}

std::string TorrentFileParser::getAnnounce() {
	return BDecoder::get<std::string>(dictionary, "announce");
}

int TorrentFileParser::getCreationDate() {
	return BDecoder::get<int>(dictionary, "creation date");
}

std::string TorrentFileParser::getComment() {
	return BDecoder::get<std::string>(dictionary, "comment");
}

std::string TorrentFileParser::getCreatedBy() {
	return BDecoder::get<std::string>(dictionary, "created by");
}

std::string TorrentFileParser::getEncoding() {
	return BDecoder::get<std::string>(dictionary, "encoding");
}

std::vector<boost::any> TorrentFileParser::getAnnounceList() {
	return BDecoder::get<std::vector<boost::any>>(dictionary, "announce-list");
}

std::unordered_map<std::string, boost::any> TorrentFileParser::getInfo() {
	return BDecoder::get<std::unordered_map<std::string, boost::any>>(dictionary, "info");
}
