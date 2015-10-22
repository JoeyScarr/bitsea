#ifndef BITSEA_CALLBACK_HPP
#define BITSEA_CALLBACK_HPP

class BitSeaCallBack {
public:
		virtual void dropPeer(std::string peerId)=0;
		virtual void taskManager()=0;
};

#endif
