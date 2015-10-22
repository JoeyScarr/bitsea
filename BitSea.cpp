#include "BitSea.hpp"

int main(int argc, char *argv[]) {
	BitSea bitsea(argc, argv);
	bitsea.run();
	
	return 0;
}

BitSea::BitSea(int argc, char **argv) : io_service(new boost::asio::io_service), work(new boost::asio::io_service::work(*io_service)) {		
	tStats.downloaded = 0;
	tStats.uploaded = 0;
	tStats.left = 0;
	
	cli::parseCommandLine(argc, argv, programSettings);
	torrentInfo = new TorrentFileParser(programSettings.fileName);
	tStats.pieceLength = torrentInfo->info.getPieceLength();
	
	allocatedPieces = new bool[tStats.numberOfPieces];
}

void BitSea::run() {
	fileList = allocateStorage();
	initPieceDatabase();
	boost::shared_ptr<Tracker> trackMan = initTracker();
	
	// Create worker threads for boost asio to farm jobs out to.
	for(int i=0; i < THREAD_MAX; i++)
		worker_threads.create_thread(boost::bind(&BitSea::WorkerThread, this, io_service));
	
	startTrackerUpdater(io_service, trackMan);
	peerList = trackMan->getPeers();
	
	// Connect to each peer given by the tracker.
	for(int i=0; i < peerList.size(); i++) {
		std::pair<std::string, int> peerPair(peerList[i].peer_id, i);
		peerResolver.insert(peerPair);
		talkToPeer(peerList[i], trackMan->getPeerId(), i);
	}
    worker_threads.join_all();
}

/* Create and initialise a db for the pieces in the torrent.
 */
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

/* Create peerclient module and connect to the clients
 */
void BitSea::talkToPeer(Tracker::Peer peerAddress, std::string peerId, int index) {
	std::string infoHash = torrentInfo->info.getHash();
	PeerAccess newPeer;
	newPeer.peer = std::make_shared<PeerClient>(PeerClient(this, io_service, peerAddress, tStats, 
						torrentInfo->info.getHash(), peerId, index, pieces, &global_stream_lock, &global_piece_lock));
	peerDb.push_back(newPeer);
	newPeer.peer->launch();
}

/* Make initial data request to the tracker for peer list etc.
 */
boost::shared_ptr<Tracker> BitSea::initTracker() {
	std::string announceUrl = torrentInfo->getAnnounce();
	std::string infoHash = torrentInfo->info.getHash();
	boost::shared_ptr<Tracker> trackMan(new Tracker(announceUrl, infoHash));
	trackMan->updateStats(tStats.downloaded, tStats.uploaded, tStats.left);
	trackMan->refresh();
	return trackMan;
}

/* Create a timer to ping the tracker every 30 minutes with updates
 */
void BitSea::startTrackerUpdater(boost::shared_ptr<boost::asio::io_service> io_service, boost::shared_ptr<Tracker> trackerManager) {
	boost::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(*io_service));
	timer->expires_from_now(boost::posix_time::minutes(30));
	timer->async_wait(boost::bind(&BitSea::trackerUpdateHandler, this, _1, timer, trackerManager));
}

/* Pre-allocate all storage for the files in the torrent. This might be
 * disabled for the meantime.  To try get torrent client up and running
 * first, we may just support single file torrents and allocate storage
 * for the continguous 'pieces'
 */
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

/* Actual callback for updating the tracker every 30 minutes.
 */
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

/* Callback for starting the various boost asio threads that handle jobs
 */
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

/* PeerClient instances will call this after certain messages are sent
 * or received, which maybe relevant to downloading pieces.
 */
void BitSea::taskManager() {
	global_taskman_lock.lock();
	if(tStats.peerHandshakesComplete >= programSettings.peerThreshold) {
		updateAvailablePieces();
		processAvailablePieces();
		allocateWork();
	}
	
	global_taskman_lock.unlock();
}

// In the future, possibly sort pieces by availability. Do nothing for
// now.
void BitSea::processAvailablePieces() {}

// Change the choke/interested status and farm out jobs up to max.
void BitSea::allocateWork() {
	int maxAllocations = std::min(programSettings.maxDownloads, (std::uint16_t) piecesAvailable.size());
	int numCurrentJobs = jobs.size();
	if(numCurrentJobs < maxAllocations) {
		for(auto piece : piecesAvailable) {
			if(!isPieceAllocated(piece)) {
				bool success = allocatePiece(piece);
				if(success)
					numCurrentJobs++;
					
				if(numCurrentJobs == maxAllocations)
					break;
			}
		}
	}
}

bool BitSea::allocatePiece(int piece) {
	std::vector<std::string> peers = pieces[piece].peers;
	for(auto peer : peers) {
		auto search = peerResolver.find(peer);
		if(search != peerResolver.end()) {
			int index = search->second;
			if(peerDb[index].peer->hasWork())
				continue;
			peerDb[index].peer->pleaseLetMeLeech(piece);
			Job currentJob;
			currentJob.piece = piece;
			currentJob.peer = index;
			jobs.insert(currentJob);
			return true;
		}
	}
	
	return false;
}

bool BitSea::isPieceAllocated(int pieceIndex) {
	if(allocatedPieces[pieceIndex])
		return true;
	else
		return false;
}

/* Try to destroy the peerclient objects - usually after we're closing
 * connection with them.  May call the sock->close() code from here in
 * the future.
 */
void BitSea::dropPeer(std::string peerId) {
	auto search = peerResolver.find(peerId);
	if(search != peerResolver.end()) {
		int peerIndex = search->second;
		peerDb[peerIndex].peer = nullptr;
		tStats.peerHandshakesComplete--;
	}
}

/* Update the db on what pieces are available after bitfield/have
 * messages.
 */
void BitSea::updateAvailablePieces() {
	for(std::unordered_set<int>::iterator it = piecesNeeded.begin(); it != piecesNeeded.end(); it++) {
		if(pieces[*it].peers.size() != 0) {
			piecesAvailable.insert(*it);
		}
	}
}

