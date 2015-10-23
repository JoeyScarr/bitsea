#ifndef BITSEA_CALLBACK_HPP
#define BITSEA_CALLBACK_HPP

class BitSeaCallBack {
public:
		virtual void dropPeer(std::string peerId)=0;
		virtual void taskManager()=0;
		virtual void writePiece(std::vector<std::uint8_t> &buffer, int pieceIndex)=0;
		virtual void completedJob(std::uint32_t piece, std::string peerId) = 0;
};

#endif
