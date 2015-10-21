#ifndef STATS_HPP
#define STATS_HPP

struct TorrentStats {
	unsigned int downloaded;
	unsigned int uploaded;
	unsigned int left;
	unsigned int numberOfPieces;
};

struct Piece {
	std::string hash;
	bool have;
	std::vector<std::string> peers;
};

#endif
