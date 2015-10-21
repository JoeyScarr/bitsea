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

	static const int COMMAND_BUFFER_SIZE = 131072;
	static const int NETWORK_BUFFER_SIZE = 2048;
	static const int CONNECTION_TIMEOUT_LIMIT = 120;
	static const int MESSAGE_OVERHEAD_LENGTH = 5;
	static const int LENGTH_PREFIX_SIZE = 4;
	static const int MESSAGE_ID_POSITION = 3;
	static const int HANDSHAKE_LENGTH = 8;
	static const int PROCESS_SUCCESS = 0;
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
		std::vector<bool> pieceAvailable;
	} peerStatus;

	struct Command {
		std::uint8_t messageId;
		std::uint32_t length;
		std::vector<uint8_t> payload;
	} commandBuffer;
	
	std::vector<std::uint8_t> processingBuffer;
	std::uint8_t networkBuffer[NETWORK_BUFFER_SIZE];
	int readBufferSize;
	std::string sendBuffer;
	std::string handshake;

	boost::shared_ptr<boost::asio::io_service> io_service;
	
	std::string ip;
	std::string port;
	std::string infoHash;
	std::string peerId;
	int peerIndex;
	
	bool processingCommand;
	int commandLength;
	
	std::vector<Piece> pieces;
		
	boost::mutex *global_stream_lock;
	boost::mutex *global_piece_lock;
	
	void onConnect(const boost::system::error_code &ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void initHandshakeMessage();
	void sendHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	bool readHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	bool verifyHandshake();
	void keepAlive();
	void choke();
	void unchoke();
	void interested();
	void notInterested();
	void decodeBitfield(std::vector<uint8_t> &data);
	std::vector<std::uint8_t> encodeBitfield();
	
	void readHandler(const boost::system::error_code& error, std::size_t bytes_transferred);
	void processCommand();
	int processMessage(std::uint8_t messageId, std::vector<uint8_t> &payload);
	void cancel(std::vector<uint8_t> &payload);
	void recvPort(std::vector<uint8_t> &payload);
	void recvHave(std::vector<uint8_t> &payload);
	void recvRequest(std::vector<uint8_t> &payload);
	void recvPiece(std::vector<uint8_t> &payload);
	void updatePieceInfo(int pieceIndex);
	void sendBitfield(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	
public:
	PeerClient(boost::shared_ptr<boost::asio::io_service> io_service, Tracker::Peer peerAddress, TorrentStats &stats, std::string infoHash, std::string peerId, int peerIndex, std::vector<Piece> pieces, boost::mutex *global_stream_lock, boost::mutex *global_piece_lock) {
		this->io_service = io_service;
		this->ip = peerAddress.ip;
		this->port = std::to_string(peerAddress.port);
		this->infoHash = infoHash;
		this->peerId = peerId;
		this->global_stream_lock = global_stream_lock;
		this->peerIndex = peerIndex;
		this->global_piece_lock	= global_piece_lock;
		this->pieces = pieces;
		status.am_choking = 1;
		status.am_interested = 0;
		status.peer_choking = 1;
		status.peer_interested = 0;
		readBufferSize = 0;
		processingCommand = false;
		commandLength = 0;
		commandBuffer.payload.reserve(COMMAND_BUFFER_SIZE);
		processingBuffer.reserve(COMMAND_BUFFER_SIZE);
		peerStatus.pieceAvailable.reserve(stats.numberOfPieces);
		for(int i=0; i < stats.numberOfPieces; i++)
			peerStatus.pieceAvailable.push_back(false);
	}

	void launch();

};


#endif
