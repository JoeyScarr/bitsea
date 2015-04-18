#ifndef INFOPARSER_HPP
#define INFOPARSER_HPP

#include "Bencoding.hpp"
#include <openssl/sha.h>


class InfoParser {
private:
	std::unordered_map<std::string, boost::any> infoDict;
	std::string pieces;
	int pieceLength;
	int privateFlag;
	std::string name;
	int length;
	std::string MD5;
	int numberOfFiles;
	std::string hash;
	
	void setPieces();
	void setPieceLength();
	void setPrivate();
	void setName();
	void setLength();
	void setMD5();
	void setFiles();
	void setHash();

public:
	std::string string;

	std::vector<boost::any> files;
	std::string getPieces();
	int getPieceLength();
	int getPrivate();
	std::string getName();
	int getLength();
	std::string getMD5();
	int getNumberOfFiles();
	void initialise(std::unordered_map<std::string, boost::any> inputDict);
	std::string getHash();
	static int fileLength(std::unordered_map<std::string, boost::any> file);
	static std::string fileMD5(std::unordered_map<std::string, boost::any> file);
	static std::string filePath(std::unordered_map<std::string, boost::any> file);
};

#endif
