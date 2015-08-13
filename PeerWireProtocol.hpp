#ifndef PEERWIREPROTOCOL_H
#define PEERWIREPROTOCOL_H

#include <array>
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <time.h>
#include <memory>
#include <vector>
#include <future>
#include <thread>
#include <chrono>
#include <asio/use_future.hpp>
#include "Tracker.hpp"
#include <algorithm>

class PeerClient {
private:

	static const struct MessageId {
		static const uint8_t choke = 0;
		static const uint8_t unchoke = 1;
		static const uint8_t interested = 2;
		static const uint8_t notInterested = 3;
		static const uint8_t have = 4;
		static const uint8_t bitfield = 5;
		static const uint8_t request = 6;
		static const uint8_t piece = 7;
		static const uint8_t cancel = 8;
		static const uint8_t port = 9;
	} mId;
	
	static const int NETWORK_BUFFER_SIZE = 1024;
	static const int CONNECTION_TIMEOUT_LIMIT = 120;
	static const int MESSAGE_OVERHEAD_LENGTH = 5;
	static const int MESSAGE_ID_POSITION = 3;
	static const int HANDSHAKE_LENGTH = 8;
	static const int COMMAND_READY = 0;
	static const int PROCESS_GET_MORE_DATA = 1;
	static const int PROCESS_READY = 0;
	
	struct Status {
		bool choked;
		bool interested;
		bool am_choking;
		bool am_interested;
		bool peer_choking;
		bool peer_interested;
		std::vector<std::uint8_t> bitfield;
	} status;
	
	struct Command {
		std::uint8_t messageId;
		int length;
		std::vector<uint8_t> payload;
	} commandBuffer;

	std::vector<uint8_t> networkBuffer;

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
		status.am_choking = 1;
		status.am_interested = 0;
		status.peer_choking = 1;
		status.peer_interested = 0;
		endpoint_iterator = resolver.resolve(query);
	}
	
	void handleNetworking();
	size_t networkRead(std::array<std::uint8_t, NETWORK_BUFFER_SIZE> &buf, asio::error_code &error);
	void processNetworkData(std::array<std::uint8_t, NETWORK_BUFFER_SIZE> buffer, size_t length); // process input from readNetworkData.
	void sendHandshake();
	bool createPeerConnection();
	bool startListener();
	void initHandshakeMessage();
	void keepAlive();
	void processMessage(std::uint8_t messageId, std::uint8_t *data, int payloadSize);
	
	void choke();
	void unchoke();
	void interested();
	void notInterested();
	void have(std::uint8_t *data, int length);
	void bitfield(std::uint8_t *data, int length);
	void request(std::uint8_t *data, int length);
	void piece(std::uint8_t *data, int length);
	void cancel(std::uint8_t *data, int length);
	void port(std::uint8_t *data, int length);
};

const int PeerClient::CONNECTION_TIMEOUT_LIMIT;

class PeerServer {
};

#endif
