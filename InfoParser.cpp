#include "InfoParser.hpp"

void InfoParser::initialise(std::unordered_map<std::string, boost::any> inputDict) {
	infoDict = inputDict;
	setFiles();
	setPieceLength();
	setPrivate();
	setPieces();
	setName();
	setLength();
	setMD5();
	setHash();
}

void InfoParser::setPieceLength() {
	try {
		pieceLength = BDecoder::get<int>(infoDict, "piece length");
	}
	catch(int e) {
		std::cerr << "Missing piece length.\n";
		exit(1);
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
		std::cerr << "Missing pieces.\n";
		exit(1);
	}
	
	if(pieces.length() % 20 != 0) {
		std::cerr << "Pieces are not the right length!\n";
		exit(1);
	}
}

void InfoParser::setName() {
	try {
		name = BDecoder::get<std::string>(infoDict, "name");
	}
	catch(int e) {
		std::cerr << "Missing filename.\n";
		exit(1);
	}
}

void InfoParser::setLength() {
	if(this->getFileMode() == FILEMODE_SINGLE) {
		try {
			length = BDecoder::get<int>(infoDict, "length");
		}
		catch(int e) {
			std::cerr << "Missing file length.\n";
			exit(1);
		}
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
		fileMode = FILEMODE_MULTI;
	}
	catch(int e) {
		fileMode = FILEMODE_SINGLE;
		numberOfFiles = 1;
	}
}

std::vector<std::string> InfoParser::getPieces() {
	std::vector<std::string> pieceList;
	int numPieces = pieces.length() / 20;
	char hash[20];
	for(int i=0; i < numPieces; i++) {
		for(int j=0; j < 20; j++) {
			hash[j] = pieces[20*i+j];
		}
		pieceList.push_back(std::string(hash));
	}
	return pieceList;
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

	if(lookup == file.end()) {
		std::cerr << "No length found.\n";
		exit(1);
	}

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
			
	if(lookup == file.end()) {
		std::cerr << "No path found.\n";
		exit(1);
	}
			
	std::vector<boost::any> fragmentedPath = boost::any_cast<std::vector<boost::any>>(lookup->second);

	std::string path;
	path += ".";
	for(boost::any pathLevel : fragmentedPath) {
		std::string strPathLevel = boost::any_cast<std::string>(pathLevel);
		path += "/";
		path += strPathLevel;
	}
	return path;
}

void InfoParser::setHash() {
	unsigned char hashString[SHA_DIGEST_LENGTH];
	const unsigned char *infoString = reinterpret_cast<const unsigned char*>(rawString.c_str());
	unsigned long length = rawString.length();
	SHA1( infoString , length, hashString);
	
	for(int i=0; i < SHA_DIGEST_LENGTH; i++)
		hash += hashString[i];
}

std::string InfoParser::getHash() {
	return hash;
}

int InfoParser::getFileMode() {
	return fileMode;
}

