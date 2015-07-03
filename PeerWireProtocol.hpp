#ifndef PEERWIREPROTOCOL_H
#define PEERWIREPROTOCOL_H

#include <array>
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include "Tracker.hpp"

class PeerClient {
private:

	struct MessageIds {
		const uint8_t choke = 0;
		const uint8_t unchoke = 1;
		const uint8_t interested = 2;
		const uint8_t notInterested = 3;
		const uint8_t have = 4;
		const uint8_t bitfield = 5;
		const uint8_t request = 6;
		const uint8_t piece = 7;
		const uint8_t cancel = 8;
		const uint8_t port = 9;
	}

	static const int NETWORK_BUFFER_SIZE = 1024;
	bool choked;
	bool interested;
	bool am_choking;
	bool am_interested;
	bool peer_choking;
	bool peer_interested;
	
	std::string sendBuffer;
	std::string handshake;
	const std::string &info_hash;
	const std::string &peer_id;
	
	asio::io_service io_service;
	asio::ip::tcp::resolver resolver;
	asio::ip::tcp::resolver::query query;
	asio::ip::tcp::socket socket;
	asio::ip::tcp::resolver::iterator endpoint_iterator;
	
public:
	PeerClient(const std::string &hash, const std::string &id, const std::string &hostname, const std::string &port)
	: info_hash(hash), peer_id(id), resolver(io_service), 
	  socket(io_service), query(hostname, port) {
		am_choking = 1;
		am_interested = 0;
		peer_choking = 1;
		peer_interested = 0;
		endpoint_iterator = resolver.resolve(query);
	}
	
	void handleNetworking();
	void processNetworkData(std::array<char, NETWORK_BUFFER_SIZE> buffer, size_t length); // process input from readNetworkData.
	void sendHandshake();
	bool createPeerConnection();
	bool startListener();
	void initHandshakeMessage();
	void keepAlive();
};

class PeerServer {
};

#endif
