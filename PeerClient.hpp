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
#include <openssl/sha.h>
#include "Tracker.hpp"
#include "Stats.hpp"
#include "BitSeaCallBack.hpp"

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

	static const int PIECE_BLOCK_SIZE = 32768; // 32KB
	static const int COMMAND_BUFFER_SIZE = 131072;
	static const int NETWORK_BUFFER_SIZE = 2048;
	static const int CONNECTION_TIMEOUT_LIMIT = 120;
	static const int MESSAGE_OVERHEAD_LENGTH = 5;
	static const int LENGTH_PREFIX_SIZE = 4;
	static const int MESSAGE_ID_POSITION = 3;
	static const int HANDSHAKE_LENGTH = 8;
	static const int PROCESS_SUCCESS = 0;
	static const int PROCESS_DROP_PEER_INVALID_MESSAGE = 2;
	
	// Maintains network status about the client from our point of view.
	struct Status {
		bool choked;
		bool interested;
		bool am_choking;
		bool am_interested;
		bool peer_choking;
		bool peer_interested;
	} status;

	// Maintains info about the pieces this client has as well as the
	// handshake string.
	struct PeerInfo {
		std::string handshake;
		std::vector<bool> pieceAvailable;
	} peerStatus;

	// Command buffer to store a complete command for processing.
	struct Command {
		std::uint8_t messageId;
		std::uint32_t length;
		std::vector<uint8_t> payload;
	} commandBuffer;
	
	std::vector<std::uint8_t> processingBuffer;
	std::uint8_t networkBuffer[NETWORK_BUFFER_SIZE];
	std::vector<std::uint8_t> pieceBuffer;
	int readBufferSize;
	std::string sendBuffer;
	std::string handshake;
	std::string ip;
	std::string port;
	std::string infoHash;
	std::string peerId;
	int peerIndex;
	bool bitfieldReceived;
	bool processingCommand;
	int commandLength;
	TorrentStats &tStats;	
	std::vector<Piece> pieces;
	bool jobInQueue;
	std::uint32_t jobPiece;
	int pieceExpectedBegin;
	bool busy;
	
	BitSeaCallBack *callback;
		
	boost::mutex *global_stream_lock;
	boost::mutex *global_piece_lock;
	
	boost::shared_ptr<boost::asio::io_service> io_service;
	boost::shared_ptr<boost::asio::ip::tcp::socket> sock;
	
	void onConnect(const boost::system::error_code &ec);
	void initHandshakeMessage();
	void sendHandshake();
	bool readHandshake();
	bool verifyHandshake();
	void recvKeepAlive();
	void recvChoke();
	void recvUnchoke();
	void recvInterested();
	void recvNotInterested();
	void recvCancel(std::vector<uint8_t> &payload);
	void recvPort(std::vector<uint8_t> &payload);
	void recvHave(std::vector<uint8_t> &payload);
	void recvRequest(std::vector<uint8_t> &payload);
	void recvPiece(std::vector<uint8_t> &payload);
	void decodeBitfield(std::vector<uint8_t> &data);
	std::vector<std::uint8_t> encodeBitfield();
	
	void readHandler(const boost::system::error_code& error, std::size_t bytes_transferred);
	void processCommand();
	int processMessage(std::uint8_t messageId, std::vector<uint8_t> &payload);

	void updatePieceInfo(int pieceIndex);
	void sendBitfield();
	void sendUnchoke();
	void sendInterested();
	void sendData(std::uint8_t *data, size_t size);
	void requestPiece(std::uint32_t piece);
	void requestPiece(std::uint32_t piece, std::uint32_t offset, std::uint32_t blockSize);
	void shutdownSequence();
	bool verifyPieceShaHash();
	void sendHave(std::uint32_t piece);
	
public:
	PeerClient(BitSeaCallBack *callback, boost::shared_ptr<boost::asio::io_service> io_service, 
		Tracker::Peer peerAddress, TorrentStats &stats, std::string infoHash, std::string peerId, int peerIndex, 
		std::vector<Piece> pieces, boost::mutex *global_stream_lock, boost::mutex *global_piece_lock
	);

	void launch();
	bool isPeerChoking();
	void pleaseLetMeLeech(int piece);
	bool hasWork();

};


#endif
