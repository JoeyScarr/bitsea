#ifndef TRACKER_HPP
#define TRACKER_HPP

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
	
	
public:
	Tracker(std::string &url);
	~Tracker();
	void sendRequest();
	void getResponse();
	void setParameter(std::string param, std::string value);
	std::string urlEncode(std::string string);

};

#endif
