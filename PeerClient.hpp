#ifndef PEERCLIENT_HPP
#define PEERCLIENT_HPP

#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include "Tracker.hpp"
#include "Stats.hpp"

class PeerClient {
private:
	boost::shared_ptr<boost::asio::io_service> io_service;
	
	std::string ip;
	std::string port;

public:
	PeerClient(boost::shared_ptr<boost::asio::io_service> io_service, Tracker::Peer peerAddress, TorrentStats &stats, std::string &infoHash, std::string peerId) {
		this->io_service = io_service;
		ip = peerAddress.ip;
		port = std::to_string(peerAddress.port);
		std::cout << "IP: " << ip << " port " << port << std::endl;
	}

};


#endif
