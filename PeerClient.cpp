#include "PeerClient.hpp"

PeerClient::PeerClient(BitSeaCallBack *callback, boost::shared_ptr<boost::asio::io_service> io_service, 
		Tracker::Peer peerAddress, TorrentStats &stats, std::string infoHash, std::string peerId, int peerIndex, 
		std::vector<Piece> pieces, boost::mutex *global_stream_lock, boost::mutex *global_piece_lock)
		: tStats(stats) {
	this->io_service = io_service;
	this->ip = peerAddress.ip;
	this->port = std::to_string(peerAddress.port);
	this->infoHash = infoHash;
	this->peerId = peerId;
	this->global_stream_lock = global_stream_lock;
	this->peerIndex = peerIndex;
	this->global_piece_lock	= global_piece_lock;
	this->pieces = pieces;
	this->callback = callback;
	status.am_choking = 1;
	status.am_interested = 0;
	status.peer_choking = 1;
	status.peer_interested = 0;
	readBufferSize = 0;
	bitfieldReceived = false;
	processingCommand = false;
	commandLength = 0;
	commandBuffer.payload.reserve(COMMAND_BUFFER_SIZE);
	processingBuffer.reserve(COMMAND_BUFFER_SIZE);
	peerStatus.pieceAvailable.reserve(stats.numberOfPieces);
	pieceBuffer.reserve(stats.pieceLength);
	for(int i=0; i < stats.numberOfPieces; i++)
		peerStatus.pieceAvailable.push_back(false);
		
	sock = boost::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(*io_service));
	jobInQueue = false;
	busy = false;
}

void PeerClient::launch() {
	try {
		boost::asio::ip::tcp::resolver resolver(*io_service);
		boost::asio::ip::tcp::resolver::query query(this->ip, this->port);
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		boost::asio::ip::tcp::endpoint endpoint = *iterator;

		global_stream_lock->lock();
		std::cout << "Connecting to: " << endpoint << std::endl;
		global_stream_lock->unlock();

		sock->async_connect(endpoint, boost::bind(&PeerClient::onConnect, this, _1));
	}
	catch(std::exception &ex) {
		global_stream_lock->lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Exception: " << ex.what() << std::endl;
		global_stream_lock->unlock();
		shutdownSequence();
	}
}

void PeerClient::shutdownSequence() {
	boost::system::error_code ec;
	sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	sock->close(ec);
	callback->dropPeer(peerId);
}

void PeerClient::onConnect(const boost::system::error_code &ec) {
	if(ec) {
		global_stream_lock->lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Error: " << ec << std::endl;
		global_stream_lock->unlock();
	}
	else {
		initHandshakeMessage();
		sendHandshake();
		if( (readHandshake()) ) {
			sock->async_read_some(boost::asio::buffer(networkBuffer, NETWORK_BUFFER_SIZE), 
									boost::bind(&PeerClient::readHandler, this, boost::asio::placeholders::error, 
									boost::asio::placeholders::bytes_transferred));
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

void PeerClient::sendHandshake() {
	try {
		boost::system::error_code error;
		boost::asio::write(*sock, boost::asio::buffer(handshake), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

bool PeerClient::readHandshake() {
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

	sendBitfield();
	
	for(int i=handshakeSize; i < readBufferSize; i++) {
		processingBuffer.push_back(networkBuffer[i]);
	}
	readBufferSize = processingBuffer.size();
	return true;
}

void PeerClient::sendBitfield() {
	std::vector<std::uint8_t> bitfield = encodeBitfield();
	int size = bitfield.size();
	std::uint8_t buf[size];
	
	for(int i=0; i < size; i++) {
		buf[i] = bitfield[i];
	}
	
	sendData(buf, size);
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
			recvKeepAlive();
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
			recvChoke();
			break;
		case mId.unchoke:
			recvUnchoke();
			break;
		case mId.interested:
			recvInterested();
			break;
		case mId.notInterested:
			recvNotInterested();
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
			recvCancel(payload);
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
	if(bitfieldReceived) {
		global_stream_lock->lock();
		std::cerr << "Error: Peer " << peerId << " trying to send us more than one bitfield!\n";
		global_stream_lock->unlock();
		//sock->close();
		// other cleanup code - destruct this peer object.
	}
	
	bitfieldReceived = true;
	
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
	
	tStats.peerHandshakesComplete++;
	callback->taskManager();
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

void PeerClient::recvChoke() {
	status.peer_choking = true;
}
	
void PeerClient::recvUnchoke() {
	status.peer_choking = false;
	if(jobInQueue) {
		jobInQueue = false;
		requestPiece(jobPiece);
	}
	
}

void PeerClient::recvInterested() {
	status.peer_interested = true;
}

void PeerClient::recvNotInterested() {
	status.peer_interested = false;
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
	if(payload.size() < 8)
		shutdownSequence();
	
	std::uint8_t index[4] = {payload[0], payload[1], payload[2], payload[3]};
	*((std::uint32_t *) index) = ntohl(*((std::uint32_t *) index));
	
	std::uint8_t begin[4] = {payload[4], payload[5], payload[6], payload[7]};
	*((std::uint32_t *) begin) = ntohl(*((std::uint32_t *) begin));
	
	if(jobPiece != *((std::uint32_t *) index) || *((std::uint32_t *) begin) != pieceExpectedBegin || payload.size()-8 != PIECE_BLOCK_SIZE ) {
		shutdownSequence();
	}
	
	for(int i=8; i<payload.size(); i++) {
		pieceBuffer.push_back(payload[i]);
	}
	
	if(payload.size() == PIECE_BLOCK_SIZE) {
		// Double check that the last piece (if it's partial) hashes the whole block including the padding at the end
		// or whether it just hashes to end of file.
		if(!verifyPieceShaHash()) {
			global_stream_lock->lock();
			std::cerr << "Sha sum failed.\n";
			global_stream_lock->unlock();
			shutdownSequence();
		}
		else {
			callback->writePiece(pieceBuffer, jobPiece);
			callback->completedJob(jobPiece, peerId);
			sendHave(jobPiece);
			busy=false;
		}
	}
	else {
		requestPiece(jobPiece, pieceBuffer.size(), PIECE_BLOCK_SIZE);
	}
}

bool PeerClient::verifyPieceShaHash() {
	std::uint8_t hashString[SHA_DIGEST_LENGTH];
	const std::uint8_t *pieceString = reinterpret_cast<const std::uint8_t*>(pieceBuffer.data());
	unsigned long length = pieceBuffer.size();
	SHA1( pieceString , length, hashString);
	
	for(int i=0; i<20; i++) {
		if(hashString[i] != pieces[jobPiece].hash[i])
			return false;
	}
	return true;
}

void PeerClient::recvCancel(std::vector<uint8_t> &payload) {
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

void PeerClient::recvKeepAlive() {
}

void PeerClient::updatePieceInfo(int index) {
	global_piece_lock->lock();
	pieces[index].peers.push_back(peerId);
	global_piece_lock->unlock();
}

bool PeerClient::isPeerChoking() {
	return status.peer_choking;
}

void PeerClient::sendUnchoke() {
	std::uint8_t message[5] = {0,0,0,1,1};
	sendData(message, 5);
}

void PeerClient::sendInterested() {
	std::uint8_t message[5] = {0,0,0,1,2};
	sendData(message, 5);
}

void PeerClient::sendData(std::uint8_t *data, size_t size) {
	try {
		boost::system::error_code error;
		boost::asio::write(*sock, boost::asio::buffer(data, size), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void PeerClient::pleaseLetMeLeech(int piece) {
	if(status.choked) {
		status.choked = false;
		sendUnchoke();
		
		status.interested = true;
		sendInterested();
		
		jobInQueue = true;
		jobPiece = piece;
	}
	else if(!isPeerChoking()) {
		requestPiece(piece);
	}
}

void PeerClient::requestPiece(std::uint32_t piece) {
	busy = true;
	requestPiece(piece, 0, PIECE_BLOCK_SIZE);
}

void PeerClient::requestPiece(std::uint32_t piece, std::uint32_t offset, std::uint32_t blockSize) {
	std::uint8_t message[17] = {0,0,0,13,6,0,0,0,0,0,0,0,0,0,0,0,0};
	
	std::uint32_t index = htonl(piece);
	*((std::uint32_t *)(message+5)) = index;
	
	std::uint32_t begin = htonl(offset);
	*((std::uint32_t *)(message+9)) = begin;
	
	std::uint32_t length = htonl(blockSize);
	*((std::uint32_t *)(message+13)) = length;
	
	pieceExpectedBegin = offset;
	sendData(message, 17);
}

bool PeerClient::hasWork() {
	if(jobInQueue || busy)
		return true;
	else
		return false;
}

void PeerClient::sendHave(std::uint32_t piece) {
	std::uint8_t message[9] = {0,0,0,5,4,0,0,0,0};
	*((std::uint32_t*) message+5) = piece;
	
	sendData(message, 9);
}
