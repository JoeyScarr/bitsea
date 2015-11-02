#!/bin/bash
#g++ -std=c++11 CommandLineParser.cpp InfoParser.cpp Bencoding.cpp InfoParser.cpp TorrentFileParser.cpp Tracker.cpp PeerWireProtocol.cpp CommandLineParser.cpp Bencodetest.cpp -pthread -lcrypto -lcurlpp -lcurl -o test 2>&1 ; ./test


g++ -std=c++11 CommandLineParser.cpp TorrentFileParser.cpp Bencoding.cpp InfoParser.cpp PeerClient.cpp Tracker.cpp BitSea.cpp -o bs -pthread -lboost_thread -lcrypto -lcurlpp -lcurl -lboost_filesystem -lboost_system -lssl
