#include "PeerClient.hpp"

void PeerClient::launch() {
	boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*io_service));
	try {
		boost::asio::ip::tcp::resolver resolver(*io_service);
		boost::asio::ip::tcp::resolver::query query(this->ip, this->port);
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		boost::asio::ip::tcp::endpoint endpoint = *iterator;

		global_stream_lock->lock();
		std::cout << "Connecting to: " << endpoint << std::endl;
		global_stream_lock->unlock();

		sock->async_connect(endpoint, boost::bind(&PeerClient::onConnect, this, _1, sock));
	}
	catch(std::exception &ex) {
		global_stream_lock->lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Exception: " << ex.what() << std::endl;
		global_stream_lock->unlock();
	}

	//boost::system::error_code ec;
	//sock->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
	//sock->close( ec );
	
}

void PeerClient::onConnect(const boost::system::error_code &ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	if(ec) {
		global_stream_lock->lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Error: " << ec << std::endl;
		global_stream_lock->unlock();
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

	sendBitfield(sock);
	
	for(int i=handshakeSize; i < readBufferSize; i++) {
		processingBuffer.push_back(networkBuffer[i]);
	}
	readBufferSize = processingBuffer.size();
	return true;
}

void PeerClient::sendBitfield(boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	std::vector<std::uint8_t> bitfield = encodeBitfield();
	int size = bitfield.size();
	std::uint8_t buf[size];
	
	for(int i=0; i < size; i++) {
		buf[i] = bitfield[i];
	}
	
	try {
		boost::system::error_code error;
		boost::asio::write(*sock, boost::asio::buffer(buf, size), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
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
		case mId.have:
			recvHave(payload);
			break;
		case mId.bitfield:
			decodeBitfield(payload);
			break;
		case mId.request:
			recvRequest(payload);
			break;
		case mId.piece:
			recvPiece(payload);
			break;
		case mId.cancel:
			cancel(payload);
			break;
		case mId.port:
			recvPort(payload);
			break;
		default:
			return PROCESS_DROP_PEER_INVALID_MESSAGE;
	}
	return PROCESS_SUCCESS;
}

void PeerClient::decodeBitfield(std::vector<uint8_t> &data) {
	std::uint8_t mask = 0b10000000;
	int numPieces = peerStatus.pieceAvailable.size();
	for(int i=0; i < data.size(); i++) {
		for(int j=0; j < 8; j++) {
			std::uint8_t comparisonMask = mask >> j;
			std::uint8_t status = comparisonMask & data[j];
			
			int index = 8*i+j;
			if(status != 0 && index < numPieces) {
				peerStatus.pieceAvailable[index] = true;
				updatePieceInfo(index);
			}
		}
	}
}

std::vector<std::uint8_t> PeerClient::encodeBitfield() {
	int numPieces = peerStatus.pieceAvailable.size();
	int numBytes = numPieces >> 3;
	if(numPieces % 8 != 0)
		numBytes++;
		
	std::vector<std::uint8_t> bitfield;
	bitfield.reserve(numBytes);
	for(int i=0; i<numBytes; i++)
		bitfield.push_back(0);
	
	std::uint8_t mask = 0b10000000;
	for(int i=0; i<numBytes; i++) {
		for(int j=0; i < 8; i++) {
			int index = 8*i+j;		
			if(index < numPieces) {
				if(peerStatus.pieceAvailable[index]) {
					int newMask = mask >> j;
					bitfield[i] = bitfield[i] | newMask;
				}
			}	
		}			
	}
	return bitfield;
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

void PeerClient::recvHave(std::vector<uint8_t> &payload) {
	if(payload.size() != 4)
		return;
		
	std::uint8_t pieceIndexArray[4];
	for(int i=0; i < 4; i++) {
		pieceIndexArray[i] = payload[i];
	}
	
	std::uint32_t index = *((std::uint32_t *) pieceIndexArray);
	index = ntohl(index);
	if(index < peerStatus.pieceAvailable.size() && !peerStatus.pieceAvailable[index]) {
		peerStatus.pieceAvailable[index] = true;
		updatePieceInfo(index);
	}
	else {
		// this is an invalid piece. do we deal with it?
	}
}

void PeerClient::recvRequest(std::vector<uint8_t> &payload) {
	// if we have the piece, and we are not over our upload cap, then
	// send that piece.
}

void PeerClient::recvPiece(std::vector<uint8_t> &payload) {
	// check that this is something we requested.
	// check SHA-1 on it.
	// save piece to file.
	// acknowledge successful receipt.
}

void PeerClient::cancel(std::vector<uint8_t> &payload) {
	std::uint8_t pieceIndexArray[4];
	std::uint8_t blockOffset[4];
	std::uint8_t blockLength[4];
	
	for(int i=0; i<4; i++) {
		pieceIndexArray[i] = payload[i];
		blockOffset[i] = payload[4+i];
		blockLength[i] = blockLength[8+i];
	}
	
	// cast these arrays to ints and do something with this info later
	// when we have job queue.
	
}

void PeerClient::recvPort(std::vector<uint8_t> &payload) {
	// Do nothing. We're not implementing DHT right now.
}

void PeerClient::keepAlive() {
}

void PeerClient::updatePieceInfo(int index) {
	global_piece_lock->lock();
	pieces[index].have = true;
	pieces[index].peers.push_back(peerId);
	global_piece_lock->unlock();
}
