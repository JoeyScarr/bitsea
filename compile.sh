#!/bin/bash
g++ -std=c++11 Bencoding.cpp InfoParser.cpp TorrentFileParser.cpp Tracker.cpp Bencodetest.cpp  -lcrypto -lcurlpp -lcurl -o test 2>&1 ; ./test
