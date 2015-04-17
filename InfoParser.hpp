#ifndef INFOPARSER_HPP
#define INFOPARSER_HPP

#include "Bencoding.hpp"

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
	
	void setPieces();
	void setPieceLength();
	void setPrivate();
	void setName();
	void setLength();
	void setMD5();
	void setFiles();

public:
	InfoParser() {}
	
	std::vector<boost::any> files;
	std::string getPieces();
	int getPieceLength();
	int getPrivate();
	std::string getName();
	int getLength();
	std::string getMD5();
	int getNumberOfFiles();
	void initialise(std::unordered_map<std::string, boost::any> inputDict);
	static int fileLength(std::unordered_map<std::string, boost::any> file);
	static std::string fileMD5(std::unordered_map<std::string, boost::any> file);
	static std::string filePath(std::unordered_map<std::string, boost::any> file);

};

#endif
