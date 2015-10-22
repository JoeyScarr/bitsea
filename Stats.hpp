#ifndef STATS_HPP
#define STATS_HPP

// Global stats which we can use to update the tracker and use to make
// decisions.
struct TorrentStats {
	unsigned int downloaded;
	unsigned int uploaded;
	unsigned int left;
	unsigned int numberOfPieces;
	unsigned int numUploads;
	unsigned int numDownloads;
	unsigned int peerHandshakesComplete;
};

// Global data structure for keeping track of which pieces we have and
// a list of peers which have that piece.
struct Piece {
	std::string hash;
	bool have;
	std::vector<std::string> peers;
};

#endif
