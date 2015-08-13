#!/bin/bash
g++ -std=c++11 Bencoding.cpp InfoParser.cpp TorrentFileParser.cpp Tracker.cpp PeerWireProtocol.cpp Bencodetest.cpp -pthread -lcrypto -lcurlpp -lcurl -o test 2>&1 ; ./test
