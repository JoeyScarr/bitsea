#ifndef TORRENTFILEPARSER_HPP
#define TORRENTFILEPARSER_HPP

#include <boost/any.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <streambuf>
#include "Bencoding.hpp"
#include "InfoParser.hpp"

class TorrentFileParser {
private:
	boost::any dictionary;
	int creationDate;
	std::vector<boost::any> announceList;
	std::string announce;
	std::string comment;
	std::string createdBy;
	std::string encoding;
	
	// Private methods
	void process();
	int getCreationDate();
	std::unordered_map<std::string, boost::any> getInfo();
	std::vector<boost::any> getAnnounceList();
	std::string getComment();
	std::string getCreatedBy();
	std::string getEncoding();

public:
	TorrentFileParser(std::string torrentFileName);
	InfoParser info;
	std::string getAnnounce();

};

#endif
