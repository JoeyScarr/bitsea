#include "Tracker.hpp"

Tracker::Tracker(std::string &url) {
	curlHandler = curl_easy_init();
	getRequest = url;
}

Tracker::~Tracker() {
	curl_easy_cleanup(curlHandler);
}

std::string Tracker::urlEncode(std::string string) {
	char *encoded = curl_easy_escape(curlHandler, string.c_str(), string.length());
	std::string result(encoded);
	curl_free(encoded);
	return result;
}

void Tracker::setParameter(std::string parameter, std::string value) {
	getRequest += "&" + parameter + "=" + urlEncode(value);
}

void Tracker::sendRequest() {
	myRequest.setOpt<curlpp::options::Url>(getRequest.c_str());
	myRequest.perform();
}
