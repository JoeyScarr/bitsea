#include "PeerClient.hpp"

void PeerClient::launch() {
	boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*io_service));
	try {
		boost::asio::ip::tcp::resolver resolver(*io_service);
		boost::asio::ip::tcp::resolver::query query(this->ip, this->port);
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		boost::asio::ip::tcp::endpoint endpoint = *iterator;

		global_stream_lock.lock();
		std::cout << "Connecting to: " << endpoint << std::endl;
		global_stream_lock.unlock();

		sock->async_connect(endpoint, boost::bind(&PeerClient::onConnect, this, _1, sock));
	}
	catch(std::exception &ex) {
		global_stream_lock.lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Exception: " << ex.what() << std::endl;
		global_stream_lock.unlock();
	}

	//boost::system::error_code ec;
	//sock->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
	//sock->close( ec );
	
}

void PeerClient::onConnect(const boost::system::error_code &ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	if(ec) {
		global_stream_lock.lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Error: " << ec << std::endl;
		global_stream_lock.unlock();
	}
	else {
		initHandshakeMessage();
		sendHandshake(sock);
		if( (readHandshake(sock)) ) {
			sock->async_read_some(boost::asio::buffer(networkBuffer, NETWORK_BUFFER_SIZE), boost::bind(&PeerClient::readHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
	}
}

void PeerClient::initHandshakeMessage() {
	const std::string pstr = "BitTorrent protocol"; // version 1.0 of BT protocol.

	char pstrlen = (char) pstr.length();
	char reserved[HANDSHAKE_LENGTH];
	for(int i=0; i < HANDSHAKE_LENGTH; i++)
		reserved[i] = '\0';
	
	handshake += pstrlen;
	handshake += pstr;
	handshake += reserved;
	handshake += infoHash;
	handshake += peerId;
}

void PeerClient::sendHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	try {
		boost::system::error_code error;
		boost::asio::write(*sock, boost::asio::buffer(handshake), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

bool PeerClient::readHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	int handshakeSize = NETWORK_BUFFER_SIZE-1;
	bool handshakeSizeSet = false;
	while(readBufferSize < handshakeSize || !handshakeSizeSet) {
		try {
			boost::system::error_code error;
			size_t readSize = sock->read_some(boost::asio::buffer(networkBuffer, NETWORK_BUFFER_SIZE), error);
			readBufferSize += readSize;
			
			if(readBufferSize > 0 && !handshakeSizeSet) {
				handshakeSizeSet = true;
				std::uint8_t size = *((std::uint8_t *)networkBuffer);
				handshakeSize = size;
			}
		}
		catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}
	
	bool handshakePassed = verifyHandshake();
	if(!handshakePassed) {
		sock->close();
		return false;
	} 

	for(int i=handshakeSize; i < readBufferSize; i++) {
		processingBuffer.push_back(networkBuffer[i]);
	}
	readBufferSize = processingBuffer.size();
	return true;
}

bool PeerClient::verifyHandshake() {
	int pstrlen = networkBuffer[0];
	int length = pstrlen + 49;
	std::string hash("");
	std::string pstr("");
	for(int i=1; i <= pstrlen; i++) {
		pstr += networkBuffer[i];
	}
	
	if(pstr != "BitTorrent protocol")
		return false;
	
	for(int i=0; i < 20; i++) {
		hash += networkBuffer[pstrlen+1+i];
	}
	
	if(hash != infoHash)
		return false;
	
	std::string id("");
	for(int i=0; i < 20; i++) {
		id += networkBuffer[pstrlen+20+8+1+i];
	}
	
	if(peerId != "" && peerId != id)
		return false;
		
	peerId = id;
	return true;
}

void PeerClient::readHandler(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if(bytes_transferred == 0)
		return;

	for(int i=0; i < bytes_transferred; i++)
		processingBuffer.push_back(networkBuffer[i]);
	
	size_t bufferSize = processingBuffer.size();
	
	if(processingCommand) {
		if(bufferSize >= LENGTH_PREFIX_SIZE+commandBuffer.length) {
			for(int i=0; i < commandBuffer.length-1; i++) {
				commandBuffer.payload.push_back(processingBuffer[i+MESSAGE_OVERHEAD_LENGTH]);
			}
			
			processingBuffer.erase(processingBuffer.begin(), processingBuffer.begin()+commandBuffer.length+LENGTH_PREFIX_SIZE);
			processCommand();
			processingCommand = false;
		}
	}
	else {
		if(bufferSize < LENGTH_PREFIX_SIZE)
			return;
			
		commandBuffer.length =*((std::uint32_t *)(networkBuffer));
		if(commandBuffer.length == 0) {
			processingBuffer.erase(processingBuffer.begin(), processingBuffer.begin()+4);
			keepAlive();
			return;
		}
		
		if(bufferSize < MESSAGE_OVERHEAD_LENGTH)
			return;
		
		commandBuffer.messageId = networkBuffer[LENGTH_PREFIX_SIZE];
		processingCommand = true;
	}
	
}

void PeerClient::processCommand() {
	// do stuff to process command
	processMessage(commandBuffer.messageId, commandBuffer.payload);
	
	commandBuffer.length = 0;
	commandBuffer.messageId = 0;
	commandBuffer.payload.clear();
}

int PeerClient::processMessage(std::uint8_t messageId, std::vector<uint8_t> &payload) {
	switch(messageId) {
		case mId.choke:
			choke();
			break;
		case mId.unchoke:
			unchoke();
			break;
		case mId.interested:
			interested();
			break;
		case mId.notInterested:
			notInterested();
			break;
		//case mId.have:
			//have(data);
			//break;
		//case mId.bitfield:
			//bitfield(data);
			//break;
		//case mId.request:
			//request(data);
			//break;
		//case mId.piece:
			//piece(data);
			//break;
		//case mId.cancel:
			//cancel(data);
			//break;
		//case mId.port:
			//port(data);
			//break;
		default:
			return PROCESS_DROP_PEER_INVALID_MESSAGE;
	}
	return PROCESS_SUCCESS;
}


void PeerClient::choke() {
	status.choked = true;
}
	
void PeerClient::unchoke() {
	status.choked = false;
}

void PeerClient::interested() {
	status.interested = true;
}

void PeerClient::notInterested() {
	status.interested = false;
}

void PeerClient::keepAlive() {
}
