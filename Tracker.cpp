#include "Tracker.hpp"

Tracker::Tracker(std::string &url, std::string &infoHash) {
	curlHandler = curl_easy_init();
	parametersSent.url = url;
	parametersSent.info_hash = infoHash;
	setPeerID();
	parametersSent.port=DEFAULT_PORT;
	parametersSent.compact=DEFAULT_COMPACT;
	parametersSent.no_peer_id=DEFAULT_NO_PEER_ID;
	parametersSent.event="started";
	parametersSent.sendEvent=true;
	parametersSent.ip="";
	parametersSent.key="";
	trackerID="";
	interval = 0;
	parametersSent.uploaded=0;
	parametersSent.downloaded=0;
	parametersSent.left=0;
	parametersSent.numwant=0;
}

Tracker::~Tracker() {
	curl_easy_cleanup(curlHandler);
}

std::string& Tracker::getPeerId() {
	return parametersSent.peer_id;
}

void Tracker::refresh() {
	constructRequest();
	std::cout << "Tracker Request: " << getRequest << std::endl;
	sendRequest();
	
	//std::cout << "Tracker Response: " << trackerRawResponse << std::endl;
	getResponse();
}

void Tracker::updateStats(unsigned int downloaded, unsigned int uploaded, unsigned int left) {
	parametersSent.uploaded = uploaded;
	parametersSent.downloaded = downloaded;
	parametersSent.left = left;
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

void Tracker::constructRequest() {
	getRequest = parametersSent.url + "?";
	getRequest += std::string("info_hash") + "=" + urlEncode(parametersSent.info_hash);
	setParameter(std::string("peer_id"), parametersSent.peer_id);
	setParameter(std::string("port"), std::to_string(parametersSent.port));
	setParameter(std::string("uploaded"), std::to_string(parametersSent.uploaded));
	setParameter(std::string("downloaded"), std::to_string(parametersSent.downloaded));
	setParameter(std::string("left"), std::to_string(parametersSent.left));

	if(parametersSent.key.compare("") != 0)
		setParameter(std::string("key"), parametersSent.key);
	
	if(parametersSent.numwant)
		setParameter(std::string("numwant"), std::to_string(parametersSent.numwant));
	
	if(parametersSent.ip.compare("") != 0)
		setParameter(std::string("ip"), parametersSent.ip);
	
	if(parametersSent.no_peer_id)
		setParameter(std::string("no_peer_id"), std::to_string(parametersSent.no_peer_id));
	
	if(parametersSent.compact)
		setParameter(std::string("compact"), std::to_string(parametersSent.compact));
		
	if(trackerID.compare("") != 0)
		setParameter(std::string("trackerid"), trackerID);
		
	if(parametersSent.sendEvent) {
		setParameter(std::string("event"), parametersSent.event);
		parametersSent.sendEvent = false;
	}
	
}

void Tracker::setPeerID() {
	parametersSent.peer_id = "-SE";
	parametersSent.peer_id += VERSION;
	parametersSent.peer_id += "-";
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);
	std::uniform_int_distribution<int> distribution(0,9);
	auto dice = std::bind ( distribution, generator );
	for(int i=0; i < 12; i++)
		parametersSent.peer_id += std::to_string(dice());
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
		parametersSent.ip = os.str();
		parametersSent.ip.erase(std::remove(parametersSent.ip.begin(), parametersSent.ip.end(), '\n'), parametersSent.ip.end());
		parametersSent.ip.erase(std::remove(parametersSent.ip.begin(), parametersSent.ip.end(), '\r'), parametersSent.ip.end());
		parametersSent.ip.erase(std::remove(parametersSent.ip.begin(), parametersSent.ip.end(), ' '), parametersSent.ip.end());
	}
	catch(curlpp::RuntimeError & e)	{
		std::cout << e.what() << std::endl;
	}
	catch(curlpp::LogicError & e) {
		std::cout << e.what() << std::endl;
	}
}

void Tracker::setPort(unsigned int port) {
	parametersSent.port = port;
}

void Tracker::setUploaded(unsigned int uploaded) {
	parametersSent.uploaded = uploaded;
}

void Tracker::setDownloaded(unsigned int downloaded) {
	parametersSent.downloaded = downloaded;
}

void Tracker::setLeft(unsigned int left) {
	parametersSent.left = left;
}

void Tracker::setCompact(unsigned int compact) {
	parametersSent.compact = compact;
}

void Tracker::setNoPeerID(unsigned int noPeerID) {
	parametersSent.no_peer_id = noPeerID;
}

void Tracker::setEvent(std::string event) {
	if(event.compare("started") == 0 || event.compare("stopped") == 0 || event.compare("completed") == 0) {
		parametersSent.event = event;
		parametersSent.sendEvent = true;
	}	
}

void Tracker::setNumWant(unsigned int numwant) {
	parametersSent.numwant = numwant;
}

void Tracker::setKey(std::string key) {
	parametersSent.key = key;
}

void Tracker::setTrackerID(std::string ID) {
	trackerID = ID;
}

void Tracker::getResponse() {
	BDecoder trackerResponse(trackerRawResponse,0);
	if(!trackerResponse.decode()) {
		std::cerr << "BDecode failed.\n";
		exit(1);
	}
	
	trackerResponseDict = trackerResponse.get();
	
	try {
		parametersReceived.failureReason = BDecoder::get<std::string>(trackerResponseDict, "failure reason");
		std::cerr << "Failure: " << parametersReceived.failureReason << std::endl;
		exit(1);
	} catch(int e) {}
	
	try {
		parametersReceived.warningMessage = BDecoder::get<std::string>(trackerResponseDict, "warning message");
		std::cout << "Warning: " << parametersReceived.warningMessage << std::endl;
	} catch(int e) {}
	
	try {
		interval = BDecoder::get<int>(trackerResponseDict, "interval");
		
	}
	catch(int e) {
		std::cerr << "No interval specified.\n";
		exit(1);
	}
	
	try {
		parametersReceived.minInterval = BDecoder::get<int>(trackerResponseDict, "min interval");
	} catch(int e) {}
	
	try {
		trackerID = BDecoder::get<std::string>(trackerResponseDict, "tracker id");
	} catch(int e) {}
	
	try {
		parametersReceived.complete = BDecoder::get<int>(trackerResponseDict, "complete");
	}
	catch(int e) {
		std::cerr << "No complete count received.\n";
		exit(1);
	}
	
	try {
		parametersReceived.incomplete = BDecoder::get<int>(trackerResponseDict, "incomplete");
	}
	catch(int e) {
		std::cerr << "No incomplete count received.\n";
		exit(1);
	}
	
	if(BDecoder::isString(trackerResponseDict, "peers")) {
		processPeerString();
	}
	else {
		processPeerDictionary();
	}
	
}

void Tracker::sendRequest() {
	try {
		curlpp::Cleanup myCleanup;
		curlpp::Easy myRequest;
		myRequest.setOpt<curlpp::options::Url>(getRequest.c_str());
		std::ostringstream os;
		curlpp::options::WriteStream ws(&os);
		myRequest.setOpt(ws);
		myRequest.perform();
		trackerRawResponse = os.str();
	}
	catch(curlpp::RuntimeError & e)	{
		std::cout << e.what() << std::endl;
	}
	catch(curlpp::LogicError & e) {
		std::cout << e.what() << std::endl;
	}
}

void Tracker::processPeerString() {
	std::string peerString;
	try {
		 peerString = BDecoder::get<std::string>(trackerResponseDict, "peers");
	}
	catch(int e) {
		std::cerr << "No peers received.\n";
		exit(1);
	}
	
	int numberOfPeers = peerString.length() / 6;
	
	for(int i=0; i < numberOfPeers; i++) {
		Tracker::Peer currentPeer;
		const char *ipString = peerString.substr(6*i, 4).c_str();
		const char *portString = peerString.substr(6*i+4, 2).c_str();
		std::uint8_t iparray[4];
		iparray[0] = peerString[6*i];
		iparray[1] = peerString[6*i+1];
		iparray[2] = peerString[6*i+2];
		iparray[3] = peerString[6*i+3];
		in_addr addy;
		addy.s_addr = *((std::uint32_t *) iparray);
		currentPeer.ip = inet_ntoa(addy);
		std::uint8_t portarray[2];
		portarray[0] = peerString[6*i+4];
		portarray[1] = peerString[6*i+5];
		currentPeer.port = *((std::uint16_t *)portarray);
		currentPeer.port = ntohs(currentPeer.port);
		currentPeer.peer_id = "";
		parametersReceived.peerList.push_back(currentPeer);
	}
}

void Tracker::processPeerDictionary() {
	std::vector<boost::any> dictList;
	try {
		 dictList = BDecoder::get<std::vector<boost::any>>(trackerResponseDict, "peers");
	}
	catch(int e) {
		std::cerr << "No peers received.\n";
		exit(1);
	}
	
	for(boost::any dict : dictList) {
		Tracker::Peer currentPeer;
		
		try {
			currentPeer.peer_id = BDecoder::get<std::string>(dict, "peer id");
		}
		catch(int e) {
			std::cerr << "No peer id received.\n";
			exit(1);
		}
		
		try {
			currentPeer.ip = BDecoder::get<std::string>(dict, "ip");
		}
		catch(int e) {
			std::cerr << "No ip received.\n";
			exit(1);
		}
		
		try {
			currentPeer.port = BDecoder::get<int>(dict, "port");
		}
		catch(int e) {
			std::cerr << "No port received.\n";
			exit(1);
		}
		
		parametersReceived.peerList.push_back(currentPeer);
	}
}

bool Tracker::constructScrapeRequest() {
	std::string hash = "";
	return constructScrapeRequest(hash);
}

bool Tracker::constructScrapeRequest(std::string &hash) {
	std::string url = parametersSent.url;
	std::string newUrl;
	std::string tail;
	unsigned int found = url.find_last_of("/");
	newUrl = url.substr(0, found+1);
	tail = url.substr(found+1);
	
	if(tail.substr(0,8).compare("announce") != 0) {
		return false;
	}
	
	boost::replace_first(tail, "announce", "scrape");
	getRequest = newUrl + tail;
	
	if(!hash.empty()) {
		setParameter(std::string("info_hash"), hash);
	}
}

void Tracker::getScrapeResponse() {
	BDecoder trackerResponse(trackerRawResponse,0);
	if(!trackerResponse.decode()) {
		std::cerr << "BDecode failed.\n";
		exit(1);
	}
	
	trackerResponseDict = trackerResponse.get();
	std::unordered_map<std::string, boost::any> fileDict = BDecoder::get<std::unordered_map<std::string, boost::any>>(trackerResponseDict, "files");
	
	try {
		parametersScrapeReceived.complete = BDecoder::get<int>(fileDict, "complete");
	}
	catch(int e) {
		std::cerr << "Missing complete field.\n";
		exit(1);
	}
	
	try {
		parametersScrapeReceived.incomplete = BDecoder::get<int>(fileDict, "incomplete");
	}
	catch(int e) {
		std::cerr << "Missing incomplete field.\n";
		exit(1);
	}
	
	try {
		parametersScrapeReceived.downloaded = BDecoder::get<int>(fileDict, "downloaded");
	}
	catch(int e) {
		std::cerr << "Missing downloaded field.\n";
		exit(1);
	}
	
	try {
		parametersScrapeReceived.name = BDecoder::get<std::string>(fileDict, "name");
	}
	catch(int e) {}
	
	try {
		parametersScrapeReceived.failureReason = BDecoder::get<std::string>(fileDict, "name");
		std::cerr << "Failure reason: " << parametersScrapeReceived.failureReason << std::endl;
		exit(1);
	}
	catch(int e) {}
	
	try {
		parametersScrapeReceived.flagsDict = BDecoder::get<std::unordered_map<std::string, boost::any>>(fileDict, "flags");
		
		try {
			parametersScrapeReceived.min_request_interval = BDecoder::get<int>(parametersScrapeReceived.flagsDict, "min_request_interval");
		}
		catch(int e) {
			std::cerr << "Missing min_request_interval.\n";
			exit(1);
		}
		
	} catch(int e) {}
}

std::vector<Tracker::Peer> Tracker::getPeers() {
	return parametersReceived.peerList;
}
