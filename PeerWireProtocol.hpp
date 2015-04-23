#ifndef PEERWIREPROTOCOL_H
#define PEERWIREPROTOCOL_H

class PeerClient {
private:
	bool choked;
	bool interested;
	bool am_choking;
	bool am_interested;
	bool peer_choking;
	bool peer_interested;
	
	std::string sendBuffer;
	
public:
	PeerClient() {	
		am_choking = 1;
		am_interested = 0;
		peer_choking = 1;
		peer_interested = 0;
	}
	
	void handshake();
};


#endif
