#include "InfoParser.hpp"

void InfoParser::initialise(std::unordered_map<std::string, boost::any> inputDict) {
	infoDict = inputDict;
	setPieceLength();
	setPrivate();
	setPieces();
	setFiles();
	setName();
	setLength();
	setMD5();
}

void InfoParser::setPieceLength() {
	try {
		pieceLength = BDecoder::get<int>(infoDict, "piece length");
	}
	catch(int e) {
		pieceLength = 0;
	}
}

void InfoParser::setPrivate() {
	try {
		privateFlag = BDecoder::get<int>(infoDict, "private");
	}
	catch(int e) {
		privateFlag = 0;
	}
}

void InfoParser::setPieces() {
	try {
		pieces = BDecoder::get<std::string>(infoDict, "pieces");
	}
	catch(int e) {
		pieces = std::string("");
	}
}

void InfoParser::setName() {
	try {
		name = BDecoder::get<std::string>(infoDict, "name");
	}
	catch(int e) {
		name = std::string("");
	}
}

void InfoParser::setLength() {
	try {
		length = BDecoder::get<int>(infoDict, "length");
	}
	catch(int e) {
		length = 0;
	}
}

void InfoParser::setMD5() {
	try {
		MD5 = BDecoder::get<std::string>(infoDict, "md5sum");
	}
	catch(int e) {
		MD5 = std::string("");
	}
}

void InfoParser::setFiles() {
	try {
		files = BDecoder::get<std::vector<boost::any>>(infoDict, "files");
		numberOfFiles = files.size();
	}
	catch(int e) {
		numberOfFiles = 1;
	}
}

std::string InfoParser::getPieces() {
	return pieces;
}

int InfoParser::getPieceLength() {
	return pieceLength;
}

int InfoParser::getPrivate() {
	return privateFlag;
}

std::string InfoParser::getName() {
	return name;
}

int InfoParser::getLength() {
	return length;
}

std::string InfoParser::getMD5() {
	return MD5;
}

int InfoParser::getNumberOfFiles() {
	return numberOfFiles;
}

int InfoParser::fileLength(std::unordered_map<std::string, boost::any> file) {
	std::string command("length");
	std::unordered_map<std::string,boost::any>::const_iterator lookup = file.find(command);
			
	if(lookup == file.end())
		throw 0;
		
	return boost::any_cast<int>(lookup->second);
}

	
std::string InfoParser::fileMD5(std::unordered_map<std::string, boost::any> file) {
	std::string command("md5sum");
	std::unordered_map<std::string,boost::any>::const_iterator lookup = file.find(command);
			
	if(lookup == file.end())
		throw 0;
		
	return boost::any_cast<std::string>(lookup->second);
}

std::string InfoParser::filePath(std::unordered_map<std::string, boost::any> file) {
	std::string command("path");
	std::unordered_map<std::string,boost::any>::const_iterator lookup = file.find(command);
			
	if(lookup == file.end())
		throw 0;
			
	std::vector<boost::any> fragmentedPath = boost::any_cast<std::vector<boost::any>>(lookup->second);

	std::string path;
	for(boost::any pathLevel : fragmentedPath) {
		std::string strPathLevel = boost::any_cast<std::string>(pathLevel);
		path += "/";
		path += strPathLevel;
	}
	return path;
}


