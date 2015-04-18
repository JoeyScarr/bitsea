#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "BitSea.hpp"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curl/curl.h>

class Tracker {
private:
	curlpp::Cleanup myCleanup;
	curlpp::Easy myRequest;
	std::string getRequest;
	CURL *curlHandler;
	
	std::string url;
	std::string info_hash;
	std::string peer_id;
	unsigned int port;
	unsigned int uploaded;
	unsigned int downloaded;
	unsigned int left;
	unsigned int compact;
	int no_peer_id;
	bool setEvent;
	std::string event;
	std::string ip;
	unsigned int numwant;
	std::string key;
	std::string trackerid;
	
	void setPeerID();
	void setIP();

public:
	Tracker(std::string &url, std::string &infoHash);
	~Tracker();
	void constructRequest();
	void sendRequest();
	void getResponse();
	void setParameter(std::string param, std::string value);
	std::string urlEncode(std::string string);
	void setPort(unsigned int port);
	void setUploaded(unsigned int uploaded);
	void setDownloaded(unsigned int downloaded);
	void setLeft(unsigned int left);
	void setCompact(unsigned int compact);
	void setNoPeerID(unsigned int noPeerID);
	void setEvent(std::string event);
	void setNumWant(unsigned int numwant);
	void setKey(std::string key);
	void setTrackerID(std::string ID);
};

#endif
