#ifndef PEERCLIENT_HPP
#define PEERCLIENT_HPP

#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/write.hpp>
#include "Tracker.hpp"
#include "Stats.hpp"

extern boost::mutex global_stream_lock;

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
	static const int PROCESS_DROP_PEER_INVALID_MESSAGE = 2;
	
	struct Status {
		bool choked;
		bool interested;
		bool am_choking;
		bool am_interested;
		bool peer_choking;
		bool peer_interested;
		std::vector<std::uint8_t> bitfield;
	} status;

	struct PeerInfo {
		std::string handshake;
	} peerStatus;

	struct Command {
		std::uint8_t messageId;
		int length;
		std::vector<uint8_t> payload;
	} commandBuffer;

	std::vector<uint8_t> networkBuffer;
	std::string sendBuffer;
	std::string handshake;

	boost::shared_ptr<boost::asio::io_service> io_service;
	
	std::string ip;
	std::string port;
	std::string infoHash;
	std::string peerId;
	
	void onConnect(const boost::system::error_code &ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void initHandshakeMessage();
	void sendHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void readHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void verifyHandshake();
	void keepAlive();
	void readHandler(const boost::system::error_code& error, std::size_t bytes_transferred);

	
public:
	PeerClient(boost::shared_ptr<boost::asio::io_service> io_service, Tracker::Peer peerAddress, TorrentStats &stats, std::string &infoHash, std::string peerId) {
		this->io_service = io_service;
		this->ip = peerAddress.ip;
		this->port = std::to_string(peerAddress.port);
		this->infoHash = infoHash;
		this->peerId = peerId;
		status.am_choking = 1;
		status.am_interested = 0;
		status.peer_choking = 1;
		status.peer_interested = 0;
	}

	void launch();

};


#endif
