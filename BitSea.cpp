#include "BitSea.hpp"


int main(int argc, char *argv[]) {
	boost::shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);
	boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*io_service));
	boost::thread_group worker_threads;
	
	cli::Settings programSettings;
	std::vector<Piece> pieces;
	std::vector<std::pair<std::string, int>> fileList;
	TorrentStats tStats;
	tStats.downloaded = 0;
	tStats.uploaded = 0;
	tStats.left = 0;
		
	cli::parseCommandLine(argc, argv, programSettings);
	TorrentFileParser torrentInfo(programSettings.fileName);	
	fileList = allocateStorage(torrentInfo, tStats);
	initPieceDatabase(torrentInfo, pieces);
	
	boost::shared_ptr<Tracker> trackMan = initTracker(torrentInfo, tStats);
	
	for(int i=0; i < THREAD_MAX; i++)
		worker_threads.create_thread(boost::bind(&WorkerThread, io_service));
	
	startTrackerUpdater(io_service, trackMan);
	std::vector<Tracker::Peer> peerList = trackMan->getPeers();
	
	for(int i=0; i < peerList.size(); i++)
		talkToPeer(io_service, peerList[i], tStats, torrentInfo, trackMan->getPeerId());
	
	//io_service->stop();
    worker_threads.join_all();

	return 0;
}

void initPieceDatabase(TorrentFileParser &torrentInfo, std::vector<Piece> &pieces) {
	std::vector<std::string> pieceHashes=torrentInfo.info.getPieces();
	for(std::vector<std::string>::iterator it = pieceHashes.begin(); it != pieceHashes.end(); it++) {
		Piece piece;
		piece.hash = *it;
		piece.have = false;
		pieces.push_back(piece);
	}
}

void talkToPeer(boost::shared_ptr<boost::asio::io_service> io_service, Tracker::Peer peerAddress, TorrentStats &stats, TorrentFileParser &torrentInfo, std::string peerId) {
	std::string infoHash = torrentInfo.info.getHash();
	PeerClient peer(io_service, peerAddress, stats, infoHash, peerId);
	peer.launch();
}

boost::shared_ptr<Tracker> initTracker(TorrentFileParser &torrentInfo, TorrentStats &stats) {
	std::string announceUrl = torrentInfo.getAnnounce();
	std::string infoHash = torrentInfo.info.getHash();
	boost::shared_ptr<Tracker> trackMan(new Tracker(announceUrl, infoHash));
	trackMan->updateStats(stats.downloaded, stats.uploaded, stats.left);
	trackMan->refresh();
	return trackMan;
}

void startTrackerUpdater(boost::shared_ptr<boost::asio::io_service> io_service, boost::shared_ptr<Tracker> trackerManager) {
	boost::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(*io_service));
	timer->expires_from_now(boost::posix_time::minutes(30));
	timer->async_wait(boost::bind(&trackerUpdateHandler, _1, timer, trackerManager));
}

std::vector<std::pair<std::string, int>> allocateStorage(TorrentFileParser &torrentInfo, TorrentStats &stats) {
	int mode = torrentInfo.info.getFileMode();
	std::vector<std::pair<std::string, int>> files;

	if(mode == InfoParser::FILEMODE_SINGLE) {
		std::string fileName = torrentInfo.info.getName();
		int length = torrentInfo.info.getLength();
		std::string md5 = torrentInfo.info.getMD5();
		files.push_back(std::make_pair(fileName, length));
	}
	else {
		for(boost::any file : torrentInfo.info.files) {
			std::unordered_map<std::string, boost::any> dict = boost::any_cast<std::unordered_map<std::string, boost::any>>(file);
			int length = InfoParser::fileLength(dict);
			std::string fileName = InfoParser::filePath(dict);
			std::string md5 = InfoParser::fileMD5(dict);
			files.push_back(std::make_pair(fileName, length));
		}
	}
	
	for(std::vector<std::pair<std::string, int>>::iterator it=files.begin(); it != files.end(); it++) {
		std::string fileName = it->first;
		int length = it->second;
		stats.left += length;
		createFile(fileName, length);
	}
	
	return files;
}

void createFile(std::string filePath, int size) {
	int pathLength = filePath.length();
	int position = 0;
	if(filePath[0] == '.' && filePath[1] == '/') {
		position = pathLength-1;
		while(filePath[position] != '/') {
			position--;
		}
		std::string directory = filePath.substr(0,position+1);
		boost::filesystem::path dir(directory);
		boost::filesystem::create_directories(dir);
		position++;
	}

	std::ofstream ofs(filePath, std::ios::binary | std::ios::out);
	ofs.seekp(size-1);
	ofs.write("",1);
	ofs.close();
}

void trackerUpdateHandler(const boost::system::error_code &error, boost::shared_ptr<boost::asio::deadline_timer> timer, boost::shared_ptr<Tracker> trackerManager) {
	if(error) {
		global_stream_lock.lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Error: " << error << std::endl;
		global_stream_lock.unlock();
	}
	else {
		trackerManager->refresh();
		timer->expires_from_now( boost::posix_time::minutes( 30 ) );
		timer->async_wait( boost::bind( &trackerUpdateHandler, _1, timer, trackerManager ) );
	}
}

void WorkerThread(boost::shared_ptr<boost::asio::io_service> io_service) {
	while(true) {
		try {
			boost::system::error_code ec;
			io_service->run(ec);
			if(ec) {
				global_stream_lock.lock();
				std::cout << "[" << boost::this_thread::get_id()
						<< "] Error: " << ec << std::endl;
				global_stream_lock.unlock();
			}
			break;
		}
		catch(std::exception & ex) {
			global_stream_lock.lock();
			std::cout << "[" << boost::this_thread::get_id()
					<< "] Exception: " << ex.what() << std::endl;
			global_stream_lock.unlock();
		}
	}
}
