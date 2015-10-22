#include "BitSea.hpp"


int main(int argc, char *argv[]) {
	BitSea bitsea(argc, argv);
	bitsea.run();
	
	return 0;
}

void BitSea::run() {
	fileList = allocateStorage();
	initPieceDatabase();
	boost::shared_ptr<Tracker> trackMan = initTracker();
	
	for(int i=0; i < THREAD_MAX; i++)
		worker_threads.create_thread(boost::bind(&BitSea::WorkerThread, this, io_service));
	
	startTrackerUpdater(io_service, trackMan);
	peerList = trackMan->getPeers();
	for(int i=0; i < peerList.size(); i++) {
		std::pair<std::string, int> peerPair(peerList[i].peer_id, i);
		peerResolver.insert(peerPair);
		talkToPeer(peerList[i], trackMan->getPeerId(), i);
	}
    worker_threads.join_all();
}

void BitSea::initPieceDatabase() {
	std::vector<std::string> pieceHashes=torrentInfo->info.getPieces();
	for(std::vector<std::string>::iterator it = pieceHashes.begin(); it != pieceHashes.end(); it++) {
		Piece piece;
		piece.hash = *it;
		piece.have = false;
		pieces.push_back(piece);
	}
	
	for(int i=0; i < pieces.size(); i++) {
		if(pieces[i].have == false) {
			piecesNeeded.insert(i);
		}
	}
	
	tStats.numberOfPieces = pieces.size();
}

void BitSea::talkToPeer(Tracker::Peer peerAddress, std::string peerId, int index) {
	std::string infoHash = torrentInfo->info.getHash();
	PeerAccess newPeer;
	newPeer.peer = std::make_shared<PeerClient>(PeerClient(this, io_service, peerAddress, tStats, torrentInfo->info.getHash(), peerId, index, pieces, &global_stream_lock, &global_piece_lock));
	peerDb.push_back(newPeer);
	newPeer.peer->launch();
}

boost::shared_ptr<Tracker> BitSea::initTracker() {
	std::string announceUrl = torrentInfo->getAnnounce();
	std::string infoHash = torrentInfo->info.getHash();
	boost::shared_ptr<Tracker> trackMan(new Tracker(announceUrl, infoHash));
	trackMan->updateStats(tStats.downloaded, tStats.uploaded, tStats.left);
	trackMan->refresh();
	return trackMan;
}

void BitSea::startTrackerUpdater(boost::shared_ptr<boost::asio::io_service> io_service, boost::shared_ptr<Tracker> trackerManager) {
	boost::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(*io_service));
	timer->expires_from_now(boost::posix_time::minutes(30));
	timer->async_wait(boost::bind(&BitSea::trackerUpdateHandler, this, _1, timer, trackerManager));
}

std::vector<std::pair<std::string, int>> BitSea::allocateStorage() {
	int mode = torrentInfo->info.getFileMode();
	std::vector<std::pair<std::string, int>> files;

	if(mode == InfoParser::FILEMODE_SINGLE) {
		std::string fileName = torrentInfo->info.getName();
		int length = torrentInfo->info.getLength();
		std::string md5 = torrentInfo->info.getMD5();
		files.push_back(std::make_pair(fileName, length));
	}
	else {
		for(boost::any file : torrentInfo->info.files) {
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
		tStats.left += length;
		createFile(fileName, length);
	}
	
	return files;
}

void BitSea::createFile(std::string filePath, int size) {
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

void BitSea::trackerUpdateHandler(const boost::system::error_code &error, boost::shared_ptr<boost::asio::deadline_timer> timer, boost::shared_ptr<Tracker> trackerManager) {
	if(error) {
		this->global_stream_lock.lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Error: " << error << std::endl;
		this->global_stream_lock.unlock();
	}
	else {
		trackerManager->refresh();
		timer->expires_from_now( boost::posix_time::minutes( 30 ) );
		timer->async_wait( boost::bind( &BitSea::trackerUpdateHandler, this, _1, timer, trackerManager ) );
	}
}

void BitSea::WorkerThread(boost::shared_ptr<boost::asio::io_service> io_service) {
	while(true) {
		try {
			boost::system::error_code ec;
			io_service->run(ec);
			if(ec) {
				this->global_stream_lock.lock();
				std::cout << "[" << boost::this_thread::get_id()
						<< "] Error: " << ec << std::endl;
				this->global_stream_lock.unlock();
			}
			break;
		}
		catch(std::exception & ex) {
			this->global_stream_lock.lock();
			std::cout << "[" << boost::this_thread::get_id()
					<< "] Exception: " << ex.what() << std::endl;
			this->global_stream_lock.unlock();
		}
	}
}

void BitSea::taskManager() {
	global_taskman_lock.lock();
	if(tStats.peerHandshakesComplete >= cli::PEER_THRESHHOLD) {
		updateAvailablePieces();
		// sort by availability? maybe implement later. for now just 
		// grab whatever is available.
		
		// farm out work. tell peers to update choke/interested status.
		// wait for them to acknowledge. then request piece we want.
		// t
	}
	
	global_taskman_lock.unlock();
}

void BitSea::dropPeer(std::string peerId) {
	auto search = peerResolver.find(peerId);
	if(search != peerResolver.end()) {
		int peerIndex = search->second;
		peerDb[peerIndex].peer = nullptr;
		tStats.peerHandshakesComplete--;
	}
}

void BitSea::updateAvailablePieces() {
	for(std::unordered_set<int>::iterator it = piecesNeeded.begin(); it != piecesNeeded.end(); it++) {
		if(pieces[*it].peers.size() != 0) {
			piecesAvailable.insert(*it);
		}
	}
}

