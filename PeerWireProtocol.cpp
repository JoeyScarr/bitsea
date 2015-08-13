#include "PeerWireProtocol.hpp"

bool PeerClient::createPeerConnection() {
	asio::connect(socket, endpoint_iterator);
	initHandshakeMessage();
	sendHandshake();
	handleNetworking();
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
	handshake += info_hash;
	handshake += peer_id;
}

void PeerClient::sendHandshake() {
	try {
		asio::error_code error;
		asio::write(socket, asio::buffer(handshake), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	keepAlive();
}

size_t PeerClient::networkRead(std::array<std::uint8_t, NETWORK_BUFFER_SIZE> &buf, asio::error_code &error) {
	return socket.read_some(asio::buffer(buf), error);
}

void PeerClient::handleNetworking() {
	try{
		std::array<std::uint8_t, NETWORK_BUFFER_SIZE> buf;
		asio::error_code error;
		size_t len;
		
		while(true) {
			std::function<size_t()> boundNetworkRead = std::bind(&PeerClient::networkRead, this, std::ref(buf), std::ref(error));
			std::future<size_t> future = std::async(boundNetworkRead);
			std::future_status status;
			status = future.wait_for(std::chrono::seconds(CONNECTION_TIMEOUT_LIMIT));
			
			if(status == std::future_status::timeout) {
				std::cerr << "Peer " + peer_id + " timed out.\n";
				break;
			}
			
			if (error == asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw asio::system_error(error); // Some other error.

			len = future.get();
			processNetworkData(buf, len);
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	socket.close();
}

int PeerClient::processCommand() {
	std::uint8_t rawLength[4];
	for(int i=0; i < 4; i++)
		rawLength[i] = networkBuffer[i];
	std::uint32_t messageLength = *reinterpret_cast<std::uint32_t *>(rawLength);
	
	messageLength = ntohl(messageLength);
	commandBuffer.length = messageLength;

	long totalCommandLength = messageLength+MESSAGE_OVERHEAD_LENGTH;

	if(messageLength == 0)
		keepAlive();

	if(networkBuffer.size() <= totalCommandLength) 
		return PROCESS_GET_MORE_DATA;
	
	std::uint8_t rawMessageId[4];
	for(int i=0; i < 4; i++)
		rawMessageId[i] = networkBuffer[MESSAGE_ID_POSITION+i];
	std::uint32_t messageId = ntohl(*reinterpret_cast<std::uint32_t *>(rawMessageId));
	commandBuffer.messageId = messageId;
	
	for(int i=MESSAGE_OVERHEAD_LENGTH; i < totalCommandLength; i++)
		commandBuffer.payload.push_back(networkBuffer[i]);
		
	processMessage();
	
	std::vector::const_iterator first = networkBuffer.begin() + totalCommandLength;
	std::vector::const_iterator last = networkBuffer.end();
	std::vector<uint8_t> remainingData(first, last);
	networkBuffer.clear();
	networkBuffer = remainingData;
}

void PeerClient::processNetworkData(std::array<std::uint8_t, NETWORK_BUFFER_SIZE> buffer, size_t length) {
	std::uint8_t *networkData = buffer.data();
	int commandStatus;
	
	for(int i=0; i < length; i++) {
		networkBuffer.push_back(networkData[i]);
	}
	
	int processStatus = PROCESS_READY;
	while(networkBuffer.size() >= MESSAGE_OVERHEAD_LENGTH && processStatus != PROCESS_GET_MORE_DATA)
		processStatus = processCommand();
}

void PeerClient::processMessage(std::uint8_t messageId, std::uint8_t *data, int payloadSize) {
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
			have(data, payloadSize);
			break;
		case mId.bitfield:
			bitfield(data, payloadSize);
			break;
		case mId.request:
			request(data, payloadSize);
			break;
		case mId.piece:
			piece(data, payloadSize);
			break;
		case mId.cancel:
			cancel(data, payloadSize);
			break;
		case mId.port:
			port(data, payloadSize);
			break;
	}
}

void PeerClient::keepAlive() {
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

void PeerClient::have(std::uint8_t *data, int length) {
	std::uint32_t piece = *reinterpret_cast<std::uint32_t *>(data);
	piece = ntohl(piece);
}

void PeerClient::bitfield(std::uint8_t *data, int length) {
	if(status.bitfield.size() != 0)
		throw "Bitfield already set for peer: " + peer_id;
		
	for(int i=0; i < length; i++) 
		status.bitfield.push_back(data[i]);
}

void PeerClient::request(std::uint8_t *data, int length) {
	std::uint32_t index = *reinterpret_cast<std::uint32_t *>(data[0]);
	std::uint32_t begin = *reinterpret_cast<std::uint32_t *>(data[4]);
	std::uint32_t len = *reinterpret_cast<std::uint32_t *>(data[8]);
	
	index = ntohl(index);
	begin = ntohl(begin);
	len = ntohl(len);
}

void PeerClient::piece(std::uint8_t *data, int length) {
	std::uint32_t index = *reinterpret_cast<std::uint32_t *>(data[0]);
	std::uint32_t begin = *reinterpret_cast<std::uint32_t *>(data[4]);
	index = ntohl(index);
	begin = ntohl(begin);
	
	std::vector<uint8_t> block;
	for(int i=0; i < length-9; i++)
		block.push_back(data[8+i]);
}

void PeerClient::cancel(std::uint8_t *data, int length) {
	std::uint32_t index = *reinterpret_cast<std::uint32_t *>(data[0]);
	std::uint32_t begin = *reinterpret_cast<std::uint32_t *>(data[4]);
	std::uint32_t len = *reinterpret_cast<std::uint32_t *>(data[8]);
	
	index = ntohl(index);
	begin = ntohl(begin);
	len = ntohl(len);
}

void PeerClient::port(std::uint8_t *data, int length) {
	std::uint32_t listenPort = *reinterpret_cast<std::uint32_t *>(data[0]);
	listenPort = ntohl(listenPort);
}
