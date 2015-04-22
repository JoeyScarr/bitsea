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
	getRequest = parametersSent.url;
	setParameter(std::string("info_hash"), parametersSent.info_hash);
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
	}
	else {
		return;
	}
	
	parametersSent.sendEvent = true;
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
	try {
		std::string peerString = BDecoder::get<std::string>(trackerResponseDict, "peers");
	}
	catch(int e) {
		std::cerr << "No peers received.\n";
		exit(1);
	}
	
	int numberOfPeers = peerString.length() / 6;
	
	for(int i=0; i < numberOfPeers; i++) {
		Tracker::peer currentPeer;
		const char *ipString = peerString.substr(6*i, 4).c_str();
		const char *portString = peerString.substr(6*i+4, 2).c_str();
		currentPeer.ip = ntohl(*ipString);
		currentPeer.port = ntohs(*portString);
		currentPeer.peer_id = "";
		parametersReceived.peerList.push_back(currentPeer);
	}
}

void Tracker::processPeerDictionary() {
	try {
		std::vector<boost::any> dictList = BDecoder::get<std::vector<boost::any>>(trackerResponseDict, "peers");
	}
	catch(int e}
		std::cerr << "No peers received.\n";
		exit(1);
	}
	
	for(boost::any dict : dictList) {
		Tracker::peer currentPeer;
		
		try {
			currentPeer.peer_id = BDecoder::get<std::string>(dict, "peer id");
		}
		catch(int e) {
			std::cerr << "No peer id received.\n";
			exit(1);
		}
		
		try {
			std::string ipString = BDecoder::get<std::string>(dict, "ip");
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
		
		struct sockaddr_in sa;
		inet_pton(AF_INET, ipString.c_str(), &(sa.sin_addr));
		currentPeer.ip = ntohl(sa.sin_addr.s_addr);
		parametersReceived.peerList.push_back(currentPeer);
	}
	
}
