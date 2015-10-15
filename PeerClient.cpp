#include "PeerClient.hpp"

void PeerClient::launch() {
	boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*io_service));
	try {
		boost::asio::ip::tcp::resolver resolver(*io_service);
		boost::asio::ip::tcp::resolver::query query(this->ip, this->port);
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		boost::asio::ip::tcp::endpoint endpoint = *iterator;

		global_stream_lock.lock();
		std::cout << "Connecting to: " << endpoint << std::endl;
		global_stream_lock.unlock();

		sock->async_connect(endpoint, boost::bind(&PeerClient::onConnect, this, _1, sock));
	}
	catch(std::exception &ex) {
		global_stream_lock.lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Exception: " << ex.what() << std::endl;
		global_stream_lock.unlock();
	}

	//boost::system::error_code ec;
	//sock->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
	//sock->close( ec );
	
}

void PeerClient::onConnect(const boost::system::error_code &ec, boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	if(ec) {
		global_stream_lock.lock();
		std::cout << "[" << boost::this_thread::get_id()
			<< "] Error: " << ec << std::endl;
		global_stream_lock.unlock();
	}
	else {
		initHandshakeMessage();
		sendHandshake(sock);
	}
}

void PeerClient::initHandshakeMessage() {
	const std::string pstr = "BitTorrent protocol"; // version 1.0 of BT protocol.

	char pstrlen = (char) pstr.length();
	char reserved[HANDSHAKE_LENGTH];
	for(int i=0; i < HANDSHAKE_LENGTH; i++)
		reserved[i] = '\0';
	
	handshake += pstrlen;
	handshake += pstr;
	handshake += reserved;
	handshake += infoHash;
	handshake += peerId;
}

void PeerClient::sendHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	try {
		boost::system::error_code error;
		boost::asio::write(*sock, boost::asio::buffer(handshake), error);
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void PeerClient::readHandshake(boost::shared_ptr<boost::asio::ip::tcp::socket> sock) {
	std::array<std::uint8_t, NETWORK_BUFFER_SIZE> buf;
	try {
		boost::system::error_code error;
		sock->async_read_some(boost::asio::buffer(buf, NETWORK_BUFFER_SIZE), boost::bind(&PeerClient::readHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void PeerClient::readHandler(const boost::system::error_code& error, std::size_t bytes_transferred) {
}

void PeerClient::keepAlive() {
}
