#ifndef BITSEA_H
#define BITSEA_H

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include <unordered_set>
#include "CommandLineParser.hpp"
#include "TorrentFileParser.hpp"
#include "Tracker.hpp"
#include "Version.hpp"
#include "PeerClient.hpp"
#include "Stats.hpp"
#include "BitSeaCallBack.hpp"

class BitSea : BitSeaCallBack {
private:
	struct PeerAccess {
		std::shared_ptr<PeerClient> peer;
	};

	boost::shared_ptr<boost::asio::io_service> io_service;
	boost::shared_ptr<boost::asio::io_service::work> work;
	boost::thread_group worker_threads;
	cli::Settings programSettings;
	std::vector<Piece> pieces;
	std::unordered_set<int> piecesNeeded;
	std::unordered_set<int> piecesAvailable;
	
	std::vector<std::pair<std::string, int>> fileList;
	TorrentStats tStats;
	TorrentFileParser *torrentInfo;
	std::vector<PeerAccess> peerDb; // refactor later to use hash table
	std::vector<Tracker::Peer> peerList;
	std::unordered_map<std::string, int> peerResolver;
	
	boost::mutex global_stream_lock;
	boost::mutex global_piece_lock;
	boost::mutex global_taskman_lock;
	
	std::vector<std::pair<std::string, int>> allocateStorage();
	void createFile(std::string path, int size);
	void test(Tracker blah);
	void trackerUpdateHandler(const boost::system::error_code &error, boost::shared_ptr<boost::asio::deadline_timer> timer, boost::shared_ptr<Tracker> trackerManager);
	void WorkerThread(boost::shared_ptr<boost::asio::io_service> io_service);
	void startTrackerUpdater(boost::shared_ptr<boost::asio::io_service> io_service, boost::shared_ptr<Tracker> trackerManager);
	boost::shared_ptr<Tracker> initTracker();
	void talkToPeer(Tracker::Peer peerAddress, std::string peerId, int index);
	void initPieceDatabase();
	void updateAvailablePieces();
	
public:
	const int THREAD_MAX = 51;

	BitSea(int argc, char **argv) : io_service(new boost::asio::io_service), work(new boost::asio::io_service::work(*io_service)) {		
		tStats.downloaded = 0;
		tStats.uploaded = 0;
		tStats.left = 0;
		
		cli::parseCommandLine(argc, argv, programSettings);
		torrentInfo = new TorrentFileParser(programSettings.fileName);
	}
	
	void run();
	void dropPeer(std::string peerId);
	void taskManager();
};







#endif
