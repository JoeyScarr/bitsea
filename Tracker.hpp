#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "Version.hpp"
#include "Bencoding.hpp"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curl/curl.h>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/any.hpp>
#include <boost/algorithm/string.hpp>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Tracker {
public:
	struct Peer {
		std::string peer_id;
		std::string ip;
		std::uint16_t port;
	};

	Tracker(std::string &url, std::string &infoHash);
	~Tracker();

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
	void refresh();
	void updateStats(unsigned int downloaded, unsigned int uploaded, unsigned int left);
	std::vector<Peer> getPeers();
	std::string& getPeerId();

private:
	static const int DEFAULT_PORT = 6881;
	static const int DEFAULT_COMPACT = 1;
	static const int DEFAULT_NO_PEER_ID = 0;
	
	struct GetRequestParameters {
	std::string url;
	std::string info_hash;
	std::string peer_id;
	unsigned int port;
	unsigned int uploaded;
	unsigned int downloaded;
	unsigned int left;
	unsigned int compact;
	int no_peer_id;
	bool sendEvent;
	std::string event;
	std::string ip;
	unsigned int numwant;
	std::string key;
};
	
	struct GetReceivedParameters {
		std::string failureReason;
		std::string warningMessage;
		int minInterval;
		int complete;
		int incomplete;
		std::vector<Peer> peerList;
	};
	
	struct GetScrapeReceivedParameters {
		int complete;
		int downloaded;
		int incomplete;
		std::string name;
		std::string failureReason;
		boost::any flagsDict;
		int min_request_interval;
	};
	
	curlpp::Cleanup myCleanup;
	curlpp::Easy myRequest;
	std::string getRequest;
	std::string trackerRawResponse;
	boost::any trackerResponseDict;
	CURL *curlHandler;
	GetRequestParameters parametersSent;
	GetReceivedParameters parametersReceived;
	GetScrapeReceivedParameters parametersScrapeReceived;
	
	std::string trackerID;
	int interval;

	void setParameter(std::string param, std::string value);
	void setPeerID();
	void setIP();
	void processPeerString();
	void processPeerDictionary();
	void constructRequest();
	bool constructScrapeRequest();
	bool constructScrapeRequest(std::string &hash);
	void sendRequest();
	void getResponse();
	void getScrapeResponse();
};


#endif
