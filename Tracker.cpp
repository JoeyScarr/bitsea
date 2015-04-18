#include "Tracker.hpp"

Tracker::Tracker(std::string &url, std::string &infoHash) {
	curlHandler = curl_easy_init();
	this->url = url;
	info_hash = infoHash;
	setPeerID();
	port=0;
	uploaded=0;
	downloaded=0;
	left=0;
	compact=0;
	no_peer_id=0;
	event="";
	sendEvent=false;
	ip="";
	numwant=0;
	key="";
	trackerid="";
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

void Tracker::constructRequest() {
	getRequest = url;
	setParameter(std::string("info_hash"), info_hash);
	setParameter(std::string("peer_id"), peer_id);
	setParameter(std::string("port"), std::to_string(port));
	setParameter(std::string("uploaded"), std::to_string(uploaded));
	setParameter(std::string("downloaded"), std::to_string(downloaded));
	setParameter(std::string("left"), std::to_string(left));
	
	if(key.compare("") != 0)
		setParameter(std::string("key"), key);
	
	if(numwant)
		setParameter(std::string("numwant"), std::to_string(numwant));
	
	if(ip.compare("") != 0)
		setParameter(std::string("ip"), ip);
	
	if(no_peer_id)
		setParameter(std::string("no_peer_id"), std::to_string(no_peer_id));
	
	if(compact)
		setParameter(std::string("compact"), std::to_string(compact));
		
	if(sendEvent)
		setParameter(std::string("event"), event);
	
	if(trackerid.compare("") != 0)
		setParameter(std::string("trackerid"), trackerid);
}

void Tracker::setPeerID() {
	peer_id = "-SE";
	peer_id += VERSION;
	peer_id += "-";
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);
	std::uniform_int_distribution<int> distribution(0,9);
	auto dice = std::bind ( distribution, generator );
	for(int i=0; i < 12; i++)
		peer_id += std::to_string(dice());
}
	
void Tracker::setIP() {
	try	{
		curlpp::Cleanup myCleanup;
		curlpp::Easy myRequest;
		myRequest.setOpt<curlpp::options::Url>("http://myexternalip.com/raw");
		std::ostringstream os;
		curlpp::options::WriteStream ws(&os);
		myRequest.setOpt(ws);
		myRequest.perform();
		ip = os.str();
		ip.erase(std::remove(ip.begin(), ip.end(), '\n'), ip.end());
		ip.erase(std::remove(ip.begin(), ip.end(), '\r'), ip.end());
		ip.erase(std::remove(ip.begin(), ip.end(), ' '), ip.end());
	}
	catch(curlpp::RuntimeError & e)	{
		std::cout << e.what() << std::endl;
	}
	catch(curlpp::LogicError & e) {
		std::cout << e.what() << std::endl;
	}
}

	//void setPort(unsigned int port);
	//void setUploaded(unsigned int uploaded);
	//void setDownloaded(unsigned int downloaded);
	//void setLeft(unsigned int left);
	//void setCompact(unsigned int compact);
	//void setNoPeerID(unsigned int noPeerID);
	//void setEvent(std::string event);
	//void setNumWant(unsigned int numwant);
	//void setKey(std::string key);
	//void setTrackerID(std::string ID);
