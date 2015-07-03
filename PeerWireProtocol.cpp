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
	char reserved[8];
	for(int i=0; i < 8; i++)
		reserved[i] = '\0';
	
	PeerClient::handshake += pstrlen;
	PeerClient::handshake += pstr;
	PeerClient::handshake += reserved;
	PeerClient::handshake += info_hash;
	PeerClient::handshake += peer_id;
}

void PeerClient::sendHandshake() {
	try {
		asio::error_code error;
		asio::write(socket, asio::buffer(handshake), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void PeerClient::handleNetworking() {
	try{
		std::array<char, NETWORK_BUFFER_SIZE> buf;
		asio::error_code error;
		size_t len;
		
		while(true) {
			len = socket.read_some(asio::buffer(buf), error);
			
			if (error == asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw asio::system_error(error); // Some other error.

			processNetworkData(buf, len);
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void PeerClient::processNetworkData(std::array<char, NETWORK_BUFFER_SIZE> buffer, size_t length) {
	char *data = buffer.data();
	std::uint8_t *networkData = reinterpret_cast<std::uint8_t *>(data);
	std::uint32_t messageLength = *reinterpret_cast<std::uint32_t *>(networkData);
	messageLength = ntohl(messageLength);
	
	if(messageLength == 0) {
		PeerClient::keepAlive();
		return;
	}
	
	std::uint8_t messageId = networkData[4];
	
}
