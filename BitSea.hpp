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
#include "CommandLineParser.hpp"
#include "TorrentFileParser.hpp"
#include "Tracker.hpp"
#include "Version.hpp"
#include "PeerClient.hpp"
#include "Stats.hpp"

struct Piece {
	std::string hash;
	bool have;
	std::vector<std::string> peers;
};

const int THREAD_MAX = 51;
boost::mutex global_stream_lock;

std::vector<std::pair<std::string, int>> allocateStorage(TorrentFileParser &torrentInfo, TorrentStats &stats);
void createFile(std::string path, int size);
void test(Tracker blah);
void trackerUpdateHandler(const boost::system::error_code &error, boost::shared_ptr<boost::asio::deadline_timer> timer, boost::shared_ptr<Tracker> trackerManager);
void WorkerThread(boost::shared_ptr<boost::asio::io_service> io_service);
void startTrackerUpdater(boost::shared_ptr<boost::asio::io_service> io_service, boost::shared_ptr<Tracker> trackerManager);
boost::shared_ptr<Tracker> initTracker(TorrentFileParser &torrentInfo, TorrentStats &stats);
void talkToPeer(boost::shared_ptr<boost::asio::io_service> io_service, Tracker::Peer peerAddress, TorrentStats &stats, TorrentFileParser &torrentInfo, std::string peerId);
void initPieceDatabase(TorrentFileParser &torrentInfo, std::vector<Piece> &pieces, TorrentStats &stats);

#endif
