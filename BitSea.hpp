#ifndef BITSEA_H
#define BITSEA_H

#define VERSION "0001"

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "CommandLineParser.hpp"
#include "TorrentFileParser.hpp"
#include "Tracker.hpp"

void allocateStorage(TorrentFileParser &torrentInfo);
void createFile(std::string path, int size);

#endif
